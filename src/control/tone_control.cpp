#include "control/tone_control.h"

bool ToneControl::apply(bool hornBusy) { return !hornBusy; }
