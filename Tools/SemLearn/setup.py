from setuptools import setup, find_packages
from pathlib import Path


here = Path(__file__).parent
long_description = (here / "README.md").read_text(encoding='utf-8')

setup(
    name='semlearn',
    version='0.1.0',
    packages=find_packages(),
    install_requires=[
        'openai',
        'requests',
        'cohere',
        'transformers',
        'sysv_ipc',
    ],
    author=' ',
    author_email=' ',
    description='Semantic learning through LLM',
    long_description=long_description,
    long_description_content_type='text/markdown',
    url=' ',
    classifiers=[
        'Programming Language :: Python :: 3',
        'License :: OSI Approved :: MIT License',
        'Operating System :: OS Independent',
    ],
    python_requires='>=3.6',
    entry_points={
        'console_scripts': [
            'semlearn=main:main',  # Update this if necessary
        ],
    },
    keywords='semantic learning LLM AI',
    license='MIT',
    project_urls={
        'Source': ' ',
        'Tracker': ' ',
    },

    package_data={
        '': ['semlearn/*.py', 'semlearn/llm_handler/*.py'],  # Adjust paths as needed
    },
    include_package_data=True,
)
