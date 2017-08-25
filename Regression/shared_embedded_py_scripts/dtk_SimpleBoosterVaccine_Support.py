class Thresholds:
    HIGH, LOW = [0.6, 0.0]

class Effects:
    PRIME, BOOST, INITIAL = [0.25, 0.45, 0.55]

class Vaccine:
    VACCINE, BOOSTER_HIGH, BOOSTER_LOW = [ -1, Thresholds.HIGH, Thresholds.LOW]

def apply_vaccine_only(cur_immunity):
    effect = cur_immunity + (1.0 - cur_immunity) * Effects.INITIAL
    return effect

def apply_booster_only(cur_immunity, threshold):
    if cur_immunity > threshold:
        effect = cur_immunity + (1.0 - cur_immunity) * Effects.BOOST
    else:
        effect = cur_immunity + (1.0 - cur_immunity) * Effects.PRIME
    return effect

def apply_vaccines(vaccines):
    effect = 0
    for vaccine in vaccines:
        if vaccine == Vaccine.VACCINE:
            effect = apply_vaccine_only(effect)
        else:
            effect = apply_booster_only(effect, vaccine)
    return effect