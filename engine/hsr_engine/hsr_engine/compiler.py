import re

class TextRuleCompiler:
    """
    Conservative text compiler.
    It converts only mechanics it can identify confidently and retains
    unsupported source text instead of inventing behavior.
    """
    STAT_MAP = {
        "atk":"atk","def":"def","spd":"spd","crit rate":"crit_rate",
        "crit dmg":"crit_dmg","break effect":"break_effect",
        "effect hit rate":"effect_hit_rate","effect res":"effect_res",
        "outgoing healing":"outgoing_healing","energy regeneration rate":"energy_regen",
        "max hp":"max_hp"
    }

    def compile_sentence(self, source, text):
        m = re.search(
            r"increases (?:the wearer's |wearer['’]s )?"
            r"(ATK|DEF|SPD|CRIT Rate|CRIT DMG|Break Effect|Effect Hit Rate|"
            r"Effect RES|Outgoing Healing|Max HP|Energy Regeneration Rate)"
            r" by (\d+(?:\.\d+)?)%", text, re.I)
        if not m:
            return {"raw_text": text}
        return {
            "kind":"stat_modifier",
            "source":source,
            "stat":self.STAT_MAP[m.group(1).lower()],
            "value":float(m.group(2))/100,
            "value_type":"percent",
            "target":"self",
            "raw_text":text
        }

    def compile_definition(self, source, text, definition_kind):
        if not text:
            return {"kind": definition_kind, "source": source, "raw_text": text}
        sentences = re.split(r"(?<=[.!?])\s+", text.replace("\n", " ").strip())
        effects = [self.compile_sentence(source, s) for s in sentences if s.strip()]
        return {
            "kind": definition_kind,
            "source": source,
            "effects": effects,
            "raw_text": text
        }
