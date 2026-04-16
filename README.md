# SemaFuzz: Semantic-oriented Fuzzing with On-the-Fly Subgraph Exploration


# Introduction

SemaFuzz is a semantic-guided fuzzing framework designed to enhance the effectiveness of software testing by integrating symbolic execution, program semantics, and language model reasoning. 
Unlike traditional fuzzers that rely solely on low-level coverage signals, SemaFuzz constructs a semantic graph of the program, enabling it to understand the intent and behavior of individual functions. 
By leveraging lightweight function-level symbolic summaries and a bottom-up semantic propagation strategy, SemaFuzz can identify underexplored yet semantically rich execution paths. 
This allows it to generate high-quality inputs that improve code coverage and expose deep bugs more efficiently than state-of-the-art fuzzers.

```
SemaFuzz
├── AFLplusplus                 -------------   the fuzzing engine
├── Benchmarks                  -------------   all the benchmarks, including setup/run scripts
├── Common                      -------------   common libraries used in the SEMAFUZZ
├── Experiments                 -------------   all the scripts for evaluation
├── LICENSE
├── SVF                         -------------   customized SVF framework
├── Tests                       -------------   unit/integrate tests for SEMAFUZZ
Tools                           -------------   tools developed for SEMAFUZZ
│   ├── buildllvm14.sh          -------------   script for building LLVM14
│   ├── buildtools.sh           -------------   script for building all the tools
│   ├── LLMProxy                -------------   adapter for local LLM agent
│   ├── SemLearn                -------------   entry for SEMAFUZZ
│   └── SymRun                  -------------   function-level symbolic execution engine
└── build.sh                    -------------   script for building whole project
```

# Installation

## 1. Requirements
SEMAFUZZ is tested on Ubuntu18.04, LLVM14.0, Python3.9.

## 2. Docker Image
docker pull ywehfasw/semafuzz:v1.1


## 3. Build SEMAFUZZ (optional with docker)
```
cd SEMAFUZZ && . buid.sh
```


## 4. Usage Example

```
# build the benchmark and run symbolic execution, constructing semantic graph
cd Benchmarks/arrow && ./build.sh all

# run SEMAFUZZ automatically for the benchmark
python -m semlearn -d -a ./
```

## 5. Experiment Scripts
5.1. start LLM agent locally (ensure that deepseek is configured && GPU support)
cd Experiment && ./startLLMServer.sh

5.2. run all experiments
Q1-Effectiveness:
SemaFuzz:
cd SemaFuzz/Experiments && ./runDocker.sh run all 4 ds-1.5b ywehfasw/semafuzz:v1.1

AFL++-LLM:
cd SemaFuzz/Experiments && ./runDocker.sh run nopath 4 ds-1.5b ywehfasw/semafuzz:v1.1
AFL++:
cd SemaFuzz/Experiments/baselines/AFL && run.sh

Q2-Efficiency:
static phase:
cd SemaFuzz/Benchmarks && ./runStaticSem.sh

runtime phase (fuzzing), refer to Q1

Q3-Ablation study:
1> different LLM
cd SemaFuzz/Experiments && ./runDocker.sh run diffllm 4 ds-14b ywehfasw/semafuzz:v1.1

2> no-llm
cd SemaFuzz/Experiments && ./runDocker.sh run nollm 4 ywehfasw/semafuzz:v1.1

3> random-sempath
cd SemaFuzz/Experiments && ./runDocker.sh run randompath 4 ds-1.5b ywehfasw/semafuzz:v1.1
