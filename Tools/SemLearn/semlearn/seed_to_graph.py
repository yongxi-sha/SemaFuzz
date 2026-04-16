#!/usr/bin/env python3
"""
seed_to_graph.py

Seed2Graph: utilities to connect LLM-related AFL++ seeds with
program graphs (subgraphs), starting from:

  - discovering LLM-lineage seeds
  - running each seed once with a given command template

Later we will extend this to:
  - collect per-seed function coverage
  - measure how each seed contributes to subgraph exploration
"""

import os
import subprocess
import shlex
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Iterable

from .seed_analysis import SeedAnalyzer, SeedInfo  # adjust import if needed


class Seed2Graph:
    """
    Seed2Graph: bridges AFL seeds and graph/subgraph analysis.

    Current capabilities:
      - Use SeedAnalyzer to find all LLM-lineage seeds.
      - Map each seed id to its queue file path.
      - Run each LLM-lineage seed once with a target command template.

    Typical usage:

        from semlearn.seed2graph import Seed2Graph

        s2g = Seed2Graph(
            bench_root=".",
            cmd_template=["./xmllint_afl", "@SEED@"]
        )
        s2g.load_llm_lineage()
        s2g.run_llm_lineage(dry_run=True)
    """

    def __init__(self, bench_root: str, cmd_template: str):
        self.bench_root = Path(os.path.abspath(bench_root))
        self.cmd_template = shlex.split(cmd_template) + ['@SEED@']

        # AFL++ layout under this benchmark
        self.queue_dir = self.bench_root / "fuzz/out/default/queue"
        self.semantic_root = self.bench_root / "fuzz/in"

        # Filled by load_llm_lineage()
        self.analyzer: Optional[SeedAnalyzer] = None
        self.id_to_path: Dict[str, Path] = {}
        self.seed_info: Dict[str, SeedInfo] = {}
        self.llm_ids: List[str] = []

        self.explored_seeds: List[str] = []

        self.func_union = set()         # all funcs in subgraphs
        self.covered_func_union = set() # funcs already explored by LLM seeds

    # ------------------------------------------------------------------ #
    # Phase 1: build LLM-lineage view
    # ------------------------------------------------------------------ #

    def load_llm_lineage(self) -> None:
        """
        Build SeedAnalyzer, compute LLM-lineage ids, and map ids to files.

        This is the "seed side" of seed→graph: we identify which queue
        entries are influenced by LLM-generated seeds.
        """
        analyzer = SeedAnalyzer(
            queue_dir=self.queue_dir,
            semantic_root=self.semantic_root,
        )
        analyzer.load()
        print(analyzer.summary())

        self.analyzer = analyzer
        self.id_to_path, self.seed_info = self._map_seed_paths(analyzer)

        # sort LLM ids by time to have deterministic order
        llm_ids = sorted(
            analyzer.get_llm_lineage_ids(),
            key=lambda sid: self._get_seed_time(sid),
        )
        self.llm_ids = llm_ids

        print(f"[Seed2Graph] bench_root: {self.bench_root}")
        print(f"[Seed2Graph] queue_dir:  {self.queue_dir}")
        print(f"[Seed2Graph] LLM-lineage seeds (ids): {len(llm_ids)}")

    # -------------------------------------------------------------- #
    # Load all semantic subgraphs via semgdump
    # -------------------------------------------------------------- #
    def _load_all_graphs(self, semgdump, graph_dir: str = "subGraphs") -> None:
        """
        Load all .dot graphs under `graph_dir` and cache their functions.

        semgdump.loadGraph(dot_path: str) is expected to return an iterable
        of function names contained in that graph.
        """
        gdir = self.bench_root / graph_dir
        if not gdir.is_dir():
            print(f"[Seed2Graph] WARNING: graph_dir not found: {gdir}")
            self.func_union = set()
            return

        graph_num = 0
        for dot_path in sorted(gdir.glob("*.dot")):
            graph_num += 1
            try:
                # call into your C-extension to load this graph
                funcs = semgdump.LoadGraph(str(dot_path))
            except Exception as e:
                print(f"[Seed2Graph] ERROR loading graph {dot_path}: {e}")
                continue

            # normalize to a set of strings
            fset = {str(f) for f in funcs}
            self.func_union.update(fset)

        print(f"[Seed2Graph] loaded {graph_num} graphs, "
              f"{len(self.func_union)} distinct functions in union")
        
    # ------------------------------------------------------------------ #
    # Check which *new* functions a seed explores
    # ------------------------------------------------------------------ #
    def explored_graph(self, func_list: Iterable[str]) -> List[str]:
        """
        Return the list of *new* functions in func_list that:
          - appear in any loaded subgraph (self.func_union), and
          - have NOT been seen before (not in self.covered_func_union).
        """
        if not self.func_union:
            # no subgraphs loaded → nothing to compare against
            return []

        Fs = []
        for f in func_list:
            if not f:
                continue
            if f in self.func_union:
                Fs.append(f)
        return Fs

    # ------------------------------------------------------------------ #
    # Phase 2: execute LLM-lineage seeds
    # ------------------------------------------------------------------ #
    def _print_progress_bar(self, done: int, total: int, sid: str = "") -> None:
        """
        Text progress bar: [#####.....] 123/8614 (1.43%) id:010022
        """
        if total <= 0:
            return

        width = 40  # characters for the bar
        ratio = done / total
        filled = int(width * ratio)
        bar = "#" * filled + "-" * (width - filled)
        percent = ratio * 100.0

        extra = f" id:{sid}" if sid else ""
        msg = f"\r[{bar}] {done}/{total} ({percent:5.2f}%)" + extra
        print(msg, end="", flush=True)


    def run_llm_lineage(
        self,
        semgdump,
        max_seeds: Optional[int] = None
    ) -> None:
        """
        Run all (or first max_seeds) LLM-lineage seeds once.

        For each seed:
          1. Execute the target on that seed.
          2. Read covered functions from semgdump.GetCoveredFuncs().
          3. Compute which of those functions are *new* wrt. subgraphs.
          4. If at least one new function appears, mark the seed effective.
        """
        if self.analyzer is None or not self.llm_ids:
            raise RuntimeError("call load_llm_lineage() before run_llm_lineage()")
        
        # load all subgraphs → populate self.func_union
        self._load_all_graphs(semgdump)
        if not self.func_union:
            raise RuntimeError("No functions loaded from subgraphs")

        ids = self.llm_ids
        if max_seeds is not None:
            ids = ids[:max_seeds]

        total = len(ids)
        print(f"[Seed2Graph] Running {total} LLM-lineage seeds")

        self.explored_seeds = []
        self.covered_func_union.clear()

        for idx, sid in enumerate(ids):
            seed_path = self.id_to_path.get(sid)
            if seed_path is None or not seed_path.is_file():
                # src-only virtual roots (e.g., 010022) have no file → skip
                continue

            # progress bar (1-based index)
            self._print_progress_bar(idx + 1, total, sid=sid)

            cmd = self._build_cmd(seed_path)
            subprocess.run(
                cmd,
                check=False,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )

            # now, we should read covered functions from shared memory
            FList = semgdump.GetCoveredFuncs() or []  # handle None defensively

            # list of *new* subgraph functions explored by this seed
            Fs = self.explored_graph(FList)
            if Fs:
                # seed is effective in function/subgraph exploration
                self.explored_seeds.append(seed_path)

                # update global coverage union
                self.covered_func_union.update(Fs)

        eff = len(self.explored_seeds)
        frac = eff / total if total > 0 else 0.0
        print(
            f"Total {eff} seeds are effective in function exploration: "
            f"{eff}/{total} ({frac:.2%}), "
            f"new explored functions: {len(self.covered_func_union)} / {len(self.func_union)}"
        )


    # ------------------------------------------------------------------ #
    # Internal helpers
    # ------------------------------------------------------------------ #

    def _map_seed_paths(
        self,
        analyzer: SeedAnalyzer,
    ) -> Tuple[Dict[str, Path], Dict[str, SeedInfo]]:
        """
        Build mapping from seed id to actual queue file path and SeedInfo.

        We scan queue/ recursively (including .state/) but keep the first
        file per id, treating it as the canonical location.
        """
        id_to_path: Dict[str, Path] = {}
        id_to_info: Dict[str, SeedInfo] = {}

        for p in self.queue_dir.rglob("id:*"):
            if not p.is_file():
                continue
            info = analyzer._parse_queue_name(p.name)
            if info is None:
                continue
            if info.sid not in id_to_path:
                id_to_path[info.sid] = p
                id_to_info[info.sid] = info

        return id_to_path, id_to_info

    def _get_seed_time(self, sid: str) -> int:
        """Return AFL time for sorting; default 0 if unknown."""
        info = self.seed_info.get(sid)
        if info is None or info.time is None:
            return 0
        return info.time

    def _build_cmd(self, seed_path: Path) -> List[str]:
        """
        Build a concrete command from the template.

        Supports '@SEED@' or '@@' as the placeholder for the seed file.
        """
        sp = str(seed_path)
        cmd: List[str] = []
        for arg in self.cmd_template:
            arg = arg.replace("@SEED@", sp).replace("@@", sp)
            cmd.append(arg)
        return cmd

    def run(self, semgdump):
        self.load_llm_lineage()
        self.run_llm_lineage(semgdump)
        
