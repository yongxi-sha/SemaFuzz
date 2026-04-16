
from .llm_api import LanguageModelAPI
from langchain_google_genai import GoogleGenerativeAI

class GeminiAPI(LanguageModelAPI):
    def __init__(self, model_name, server_url, max_retries=3, min_interval=0.0):
        """
        :param api_key: API key for the Gemini model.
        :param max_retries: Maximum number of retries for failed queries.
        :param min_interval: Minimum time interval between requests.
        """
        super().__init__(model_name, max_retries, min_interval)

        self.api_key = "AIzaSyAVj5UemRqtzk7rqCvngH1wQLTtMlE2XWA"
        self.server_url = server_url

        self.llm = GoogleGenerativeAI(
            model=model_name,
            google_api_key=self.api_key,
            temperature=0
        )

    

