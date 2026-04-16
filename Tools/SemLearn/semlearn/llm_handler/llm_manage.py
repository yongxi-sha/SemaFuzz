from abc import ABC, abstractmethod
import sys
#from .openai_api import OpenAIAPI
from .gemini_api import GeminiAPI
from .deepseek_api import DeepSeekAPI

class LLMManage:
    def __init__(self, model_name="ds-remote"):
        """
        :param model_name: Name of the default active model.
        """
        self.active_model = None
        self.name_to_model = {}

        # Initialize supported models
        self.name_to_model["ds-1.5b"] = DeepSeekAPI(
            model_name="deepseek-r1:1.5b",
            server_url="http://ollama-server:5001"
        )

        self.name_to_model["ds-7b"] = DeepSeekAPI(
            model_name="deepseek-r1:7b",
            server_url="http://ollama-server:5001"
        )

        self.name_to_model["ds-14b"] = DeepSeekAPI(
            model_name="deepseek-r1:14b",
            server_url="http://ollama-server:5001"
        )

        self.name_to_model["ds-32b"] = DeepSeekAPI(
            model_name="deepseek-r1:32b",
            server_url="http://ollama-server:5001"
        )

        self.name_to_model["ds-remote"] = DeepSeekAPI(
            model_name="deepseek-chat",
            server_url="https://api.deepseek.com"
        )

        self.name_to_model["gemini"] = GeminiAPI(
            model_name="gemini-pro",
            server_url=None
        )

        # Set the active model
        self.setActiveModel(model_name)

    def setActiveModel(self, model_name):
        """
        Sets the active model.
        """
        model = self.name_to_model.get(model_name)
        if model is None:
            raise ValueError(f"Model '{model_name}' is not supported.")
        self.active_model = model

    def getActiveModel(self):
        """
        Returns the currently active model.
        """
        return self.active_model

    def showAvailableModels(self):
        """
        Prints a list of available models.
        """
        print("\n<<<<<< Available Models >>>>>>")
        for name, model in self.name_to_model.items():
            print(f"    --model: {name}", end="")
            if model == self.getActiveModel ():
                print ("\t>>>>>>\tActive")
            else:
                print("")
        print("\n")

