class ProceduralMacroContext:
    def __init__(self):

        # Storage for all procedural macro rules.
        self.rules = {}

    def add_rule(self, name, rule):
        self.rules[name] = rule

    def get_rule(self, name):
        return self.rules[name]
