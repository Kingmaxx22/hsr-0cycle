import json
from pathlib import Path

class DataLoader:
    def __init__(self, data_dir):
        self.data_dir = Path(data_dir)

    def load_json(self, name):
        return json.loads((self.data_dir / name).read_text(encoding="utf-8"))

    def relic_sets(self): return self.load_json("relic_sets_rules.json")
    def characters(self): return self.load_json("characters_rules.json")
    def light_cones(self): return self.load_json("light_cones_rules.json")
    def monsters(self): return self.load_json("monsters_rules.json")
    def character_skills(self): return self.load_json("character_skills_rules.json")
    def character_major_traces(self): return self.load_json("character_major_traces_rules.json")
