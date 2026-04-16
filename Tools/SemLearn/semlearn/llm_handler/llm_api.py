from abc import ABC, abstractmethod
import time
import re
from langchain_core.prompts import PromptTemplate
from langchain_core.output_parsers import JsonOutputParser
from pydantic import BaseModel, Field

class LanguageModelAPI(ABC):
    def __init__(self, model_name, max_retries=3, min_interval=0.0):
        """
        :param model_name: Name of the model.
        :param max_retries: Maximum number of retries for failed queries.
        :param min_interval: Minimum time interval between requests (for rate limiting).
        """
        self.model_name = model_name
        self.max_retries = max_retries
        self.min_interval = min_interval
        self._last_call_time = 0
        self.llm = None

    def getName(self):
        """Returns the name of the model."""
        return self.model_name
    
    class StrResult(BaseModel):
        result: str = Field(description="Result for string output")

    def query(self, question, format=StrResult, system_prompt=""):
        """
        Handles common query logic (retries, rate limiting, etc.).
        Delegates model-specific logic to the `_query` method.
        """
        now = time.time()
        elapsed = now - self._last_call_time
        if elapsed < self.min_interval:
            time.sleep(self.min_interval - elapsed)

        self._last_call_time = time.time()
        template = "System: {system_instruction}\n\nQuestion: {question}\nOutput format: {format_instructions}\n"

        try_count = 0
        while try_count < self.max_retries:
            try:
                parser = JsonOutputParser (pydantic_object=format)
                prompt = PromptTemplate(
                    template=template,
                    input_variables=["question"],
                    partial_variables={
                        "system_instruction": system_prompt,
                        "format_instructions": parser.get_format_instructions()
                    }
                )

                chain = prompt | self.llm | parser
                return chain.invoke({"question": question})
            
            except Exception as e:
                print(f"{self.model_name} API Error: {e}")
                try_count += 1
                print(f"Retry #{try_count}")

        print(f"Query failed after {self.max_retries} retries.")
        return None

