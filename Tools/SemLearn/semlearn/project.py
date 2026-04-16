import os
import yaml
import random

class Project():
    def __init__(self, ymalFile):
        self.ymalFile = ymalFile

        self.stcInfo = ""
        # self.dynInfo = ""
        self.dynInfo_format = ""
        self.dynInfo_output = ""

        self.codingType = ""

    def getValue (self, yamlData, field):
        if field in yamlData:
            return yamlData[field]
        else:
            return ""

    def getProjInfo (self, yamlData, field):
        if field in yamlData:
            return yamlData[field]
        else:
            return ""

    def initProfile (self):
        try:
            with open(self.ymalFile) as f:
                yamlData = yaml.safe_load(f)
                projData = yamlData ['project']

                proj_name   = self.getValue (projData, 'name')
                proj_des    = self.getValue (projData, 'description')
                proj_github = self.getValue (projData, 'github') 
                proj_format = self.getValue (projData, 'input_format')
                test_format = self.getValue (projData, 'test_format')
                test_des    = self.getValue (projData, 'test_des')
                test_encoding = self.getValue (projData, 'test_encoding')
        except Exception as e:
                print(f"load yaml file error: {self.ymalFile} --> {e}")
                print(os.getcwd())
                return False

        self.codingType = test_encoding
        self.stcInfo = f""" Project information: {proj_name}@{proj_github} ({proj_des})."""  

        # self.dynInfo = f""" Generate a suitable inputs (format: {proj_format}) for {proj_name}@{proj_github} ({proj_des}) to trigger the given function call path represented in json with nodes and edges."""
        self.dynInfo_format = f"""Generate a minimal valid {test_format} file as raw file data (represented as a byte stream or text string).\n
        It must:\n
        1.Follow the file structure:\n{test_des}
        2.The data should be specifically crafted to exercise the following call path:\n
        """

        randSize = random.randint(32, 1024)

        self.dynInfo_output = f"""
        3. Output must be **only** the {test_encoding} encoding of the {test_format} file without any additional explanations, reasoning, or labels.\n
        4. The data should be truly minimal without unnecessary padding or redundant 0x00 bytes. The length should be just enough (around {randSize} bytes) for a {test_format} file structure.\n\n
Output:
  Please provide {test_format} contents (as raw file data, e.g. a byte stream for a .mp3 file) with encoding of {test_encoding} to trigger the graph above\n"""

        ####################################################################################
        ####################################################################################
        return True


    def getStcProfile (self):
        return self.stcInfo
    
    def getDynProfile (self, call_path):
        dynInfo = self.dynInfo_format + f"{call_path}\n" + self.dynInfo_output
        return dynInfo
    
    def isEncodingBase64 (self):
        print (f"@@isEncodingBase64: {self.codingType}")
        if self.codingType == "base64":
            return True
        else:
            return False
