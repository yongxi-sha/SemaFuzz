#!/usr/bin/env python3
"""
seed_analysis.py

Analyze how many AFL++ queue seeds are mutated from LLM-generated seeds.

Assumptions:
  - All LLM-generated seeds are stored under:
        <semantic_root>/semantic*/   (multiple directories)
  - AFL++ queue filenames look like:
        id:000123,orig:foo.xml,src:000045,op:havoc,rep:8

We treat a queue seed as an "LLM root" if its `orig:` basename is one of the
files under semantic* dirs. Any seed whose ancestor chain (via `src:`) includes
an LLM root is considered LLM-lineage.
"""
import os
import re
from pathlib import Path
from dataclasses import dataclass
from typing import Dict, Optional, Set, Tuple

QUEUE_RE = re.compile(
    r"id:(\d+)"
    r"(?:,orig:([^,]+))?"
    r"(?:,src:(\d+))?"
)


@dataclass
class SeedInfo:
    sid: str             # e.g., "000123"
    orig: str            # original file (may be "")
    src: Optional[str]   # parent id, or None
    time: Optional[int]  # AFL 'time' field, or None


class SeedAnalyzer:
    """
    Analyze AFL++ queue seeds with respect to LLM-generated seeds.
    """

    def __init__(self, queue_dir: Path, semantic_root: Path):
        self.queue_dir = queue_dir
        self.semantic_root = semantic_root

        self.llm_basenames: Set[str] = set()
        self.nodes: Dict[str, SeedInfo] = {}
        self.is_llm_root: Dict[str, bool] = {}
        self.is_llm_lineage: Dict[str, bool] = {}

    # ------------------------------------------------------------------ #
    # Core steps
    # ------------------------------------------------------------------ #

    def load(self) -> None:
        """Load LLM basenames and AFL queue nodes."""
        self.llm_basenames = self._collect_llm_basenames()
        self.nodes = self._build_nodes()
        self.is_llm_root, self.is_llm_lineage = self._compute_lineage()

    def summary(self) -> str:
        """Return a human-readable summary string."""
        total = len(self.nodes)
        root_count = sum(1 for v in self.is_llm_root.values() if v)
        lineage_count = sum(1 for v in self.is_llm_lineage.values() if v)

        evolved_ids = self.get_evolved_from_llm_ids()
        evolved_count = len(evolved_ids)

        if total == 0:
            frac_lineage = 0.0
            frac_evolved = 0.0
        else:
            frac_lineage = lineage_count / total
            frac_evolved = evolved_count / total

        virtual_root_count = len(getattr(self, "virtual_llm_roots", set()))

        lines = [
            f"Queue directory:                 {self.queue_dir}",
            f"Semantic root:                   {self.semantic_root}",
            f"LLM seed files found:            {len(self.llm_basenames)}",
            f"Semantic LLM roots (queue ids):  {root_count}",
            f"Virtual LLM roots (src-only ids):{virtual_root_count}",
            f"Total queue seeds:               {total}",
            f"Seeds in LLM lineage (incl. roots): {lineage_count}"
            f"  → fraction: {lineage_count}/{total} ({frac_lineage:.2%})",
            f"Seeds evolved from LLM (non-roots): {evolved_count}"
            f"  → fraction: {evolved_count}/{total} ({frac_evolved:.2%})",
        ]
        return "\n".join(lines)


    # ------------------------------------------------------------------ #
    # Accessors for further analysis
    # ------------------------------------------------------------------ #

    def get_llm_root_ids(self) -> Set[str]:
        """Return ids of seeds that are direct LLM roots."""
        return {sid for sid, v in self.is_llm_root.items() if v}

    def get_llm_lineage_ids(self) -> Set[str]:
        """Return ids of seeds that are in the LLM lineage."""
        return {sid for sid, v in self.is_llm_lineage.items() if v}

    # ------------------------------------------------------------------ #
    # Internal helpers
    # ------------------------------------------------------------------ #

    def get_evolved_from_llm_ids(self) -> Set[str]:
        """
        Return ids of seeds that evolved from LLM-generated seeds:
        they are in the LLM lineage but are not direct LLM roots.
        (Note: descendants of virtual roots are all counted here since
        virtual roots have no queue entry.)
        """
        evolved: Set[str] = set()
        for sid in self.nodes.keys():
            if self.is_llm_lineage.get(sid, False) and not self.is_llm_root.get(sid, False):
                evolved.add(sid)
        return evolved


    def _collect_llm_basenames(self) -> Set[str]:
        """
        Collect basenames of all files under semantic* directories.

        e.g. fuzz/in/semantic0/, fuzz/in/semantic1/, ...
        """
        if not self.semantic_root.is_dir():
            raise FileNotFoundError(f"semantic root not found: {self.semantic_root}")

        names: Set[str] = set()

        for subdir in sorted(self.semantic_root.iterdir()):
            if not subdir.is_dir():
                continue
            if not subdir.name.startswith("semantic"):
                continue

            for f in subdir.iterdir():
                if f.is_file():
                    names.add(f.name)

        return names

    def _build_nodes(self) -> Dict[str, SeedInfo]:
        root = Path(self.queue_dir)
        if not root.is_dir():
            raise FileNotFoundError(f"queue dir not found: {self.queue_dir}")

        nodes: Dict[str, SeedInfo] = {}

        # Recursively scan queue/ and its subdirectories (including .state/)
        for p in root.rglob("*"):
            if not p.is_file():
                continue
            info = self._parse_queue_name(p.name)
            if info is None:
                continue

            # If the same id appears multiple times (queue + .state), keep first
            if info.sid not in nodes:
                nodes[info.sid] = info

        return nodes


    @staticmethod
    def _parse_queue_name(name: str) -> Optional[SeedInfo]:
        """
        Parse a queue filename like:
          id:000005,src:000003,time:10,execs:68,op:quick,pos:0,+cov
          id:000000,time:0,execs:0,orig:attrib.xml

        into SeedInfo.
        """
        parts = name.split(',')
        kv = {}
        for part in parts:
            if ':' not in part:
                continue
            k, v = part.split(':', 1)
            kv[k] = v

        if "id" not in kv:
            return None

        sid = kv["id"]
        orig = kv.get("orig", "")
        src = kv.get("src")  # may be None
        time_str = kv.get("time")
        time_val = int(time_str) if time_str is not None and time_str.isdigit() else None

        return SeedInfo(sid=sid, orig=orig, src=src, time=time_val)

    def _compute_lineage(self):
        """
        LLM roots and lineage:

          - Semantic roots:
              seeds whose `orig` basename is one of the files under semantic*/.
          - Virtual LLM roots:
              IDs that appear as `src:` but never appear as `id:` in any node
              (e.g., 010022 in your example).

        A seed is LLM-lineage if:
          - it is a semantic root, OR
          - its ancestor chain via `src:` eventually reaches a virtual LLM root.
        """

        # --- 1) Pre-compute real IDs and all src IDs ---
        real_ids = set(self.nodes.keys())
        src_ids = {info.src for info in self.nodes.values() if info.src is not None}

        # IDs that are only ever referenced as src, never as id → virtual LLM roots
        virtual_llm_roots: Set[str] = src_ids - real_ids

        # --- 2) Semantic roots: nodes whose orig is in semantic*/ ---
        is_llm_root: Dict[str, bool] = {}
        for sid, info in self.nodes.items():
            if info.orig in self.llm_basenames:
                is_llm_root[sid] = True
            else:
                is_llm_root[sid] = False

        # --- 3) DFS to compute lineage with both kinds of roots ---
        is_llm_lineage: Dict[str, bool] = {}

        def dfs(sid: str) -> bool:
            if sid in is_llm_lineage:
                return is_llm_lineage[sid]

            info = self.nodes[sid]

            # Case A: semantic root itself
            if is_llm_root[sid]:
                is_llm_lineage[sid] = True
                return True

            parent = info.src

            # Case B: no parent and not a semantic root → non-LLM root
            if parent is None:
                is_llm_lineage[sid] = False
                return False

            # Case C: parent exists as a normal node → recurse
            if parent in self.nodes:
                res = dfs(parent)
                is_llm_lineage[sid] = res
                return res

            # Case D: parent is missing as a node, but appears as src-only
            #         → parent is a virtual LLM root (e.g., 010022)
            if parent in virtual_llm_roots:
                is_llm_lineage[sid] = True
            else:
                is_llm_lineage[sid] = False

            return is_llm_lineage[sid]

        for sid in real_ids:
            dfs(sid)

        # We still want a "root count" for the summary: all semantic roots.
        # (Virtual roots are not nodes, so they don't have SeedInfo.)
        self.virtual_llm_roots = virtual_llm_roots  # store for summary/debug
        return is_llm_root, is_llm_lineage

def analyzeSeeds(bench_path: str) -> None:
    bench = os.path.abspath(bench_path)

    queue_path = Path(os.path.join(bench, "fuzz/out/default/queue"))
    llm_seed_path = Path(os.path.join(bench, "fuzz/in"))

    analyzer = SeedAnalyzer(
        queue_dir=queue_path,
        semantic_root=llm_seed_path
    )

    analyzer.load()
    print(analyzer.summary())