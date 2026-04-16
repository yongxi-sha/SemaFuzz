from .llm_api import LanguageModelAPI
from langchain_openai import ChatOpenAI
from langchain_core.prompts import PromptTemplate
from langchain_core.output_parsers import JsonOutputParser

class DeepSeekAPI(LanguageModelAPI):
    def __init__(self, model_name, server_url, max_retries=3, min_interval=0.0):
        """
        :param model_name: Name of the model (e.g., "DeepSeek-1.5B").
        :param server_url: URL of the DeepSeek model server (local or remote).
        :param max_retries: Maximum number of retries for failed queries.
        :param min_interval: Minimum time interval between requests.
        """
        super().__init__(model_name, max_retries, min_interval)

        self.api_key = "sk-7a43756ae3c847f4b3f45f7b0a98df4f"
        self.server_url = server_url

        self.llm = ChatOpenAI(
            model=model_name,
            openai_api_key=self.api_key,
            openai_api_base=self.server_url,
            max_tokens=8192,
            temperature=0
        )

    