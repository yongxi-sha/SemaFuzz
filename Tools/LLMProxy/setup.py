from setuptools import setup, find_packages

setup(
    name="llmproxy",
    version="0.1.0",
    description="A FastAPI bridge server that starts Ollama on a random port and exposes an OpenAI-compatible API.",
    long_description=(
        "This tool automatically starts an Ollama server on a random unused port (if supported) "
        "and then runs a FastAPI bridge server on a user-specified port. The bridge exposes an endpoint "
        "compatible with OpenAI's ChatCompletion API, allowing you to point your client code at it "
        "via the 'openai_api_base' parameter."
    ),
    author="xxxxx",
    author_email="xxxxx",
    url="https://github.com/llmproxy",
    packages=find_packages(),
    install_requires=[
        "fastapi",
        "uvicorn",
        "requests"
    ],
    entry_points={
        "console_scripts": [
            "ollama-bridge=ollama_bridge.bridge:main",
        ]
    },
    classifiers=[
        "Programming Language :: Python :: 3",
        "Operating System :: OS Independent",
    ],
    python_requires=">=3.7",
)
