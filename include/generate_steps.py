#!/usr/bin/env python3

import math

header = """
#ifndef STEPS_H
#define STEPS_H

#include <vector>

static const std::vector<uint16_t> kSteps = {
  // idx         db  diff  diff_from_selected
"""

footer = """
};

#endif // STEPS_H
"""

digital_pot_total_resistance = 5000.0
digital_pot_steps = 256
input_resistor = 1000.0
speaker_input_resistance = 600.0
target_step_db = 2
print_extra_step_db = 1.0

each_step = digital_pot_total_resistance / digital_pot_steps

last_chosen_step_db = 0
last_step_db = 0
num_chosen = 1

def db_for_step(step):
    step_resistance = each_step * step
    step_db = 20 * math.log10(step_resistance / (step_resistance + input_resistor * 2 + speaker_input_resistance))
    return step_db

print(header)
lines = []
for i in range(digital_pot_steps, 0, -1):
    step_db = db_for_step(i)
    difference = abs(step_db - last_step_db)
    chosen_difference = abs(step_db - last_chosen_step_db)
    distance_from_target = abs(target_step_db - chosen_difference)
    if i > 1:
        next_step_db = abs(db_for_step(i - 1) - last_chosen_step_db)
        next_distance_from_target = abs(target_step_db - next_step_db)
    else:
        next_step_db = 0
        next_distance_from_target = 100

    if chosen_difference >= target_step_db or (distance_from_target < next_distance_from_target):
        last_chosen_step_db = step_db
        num_chosen += 1
        lines.append(f'     {i:3d}, // {step_db:6.2f}  {difference:3.2f}  {chosen_difference:3.2f}')
    else:
        lines.append(f'  // {i:3d}, // {step_db:6.2f}  {difference:3.2f}  {chosen_difference:3.2f}')

    last_step_db = step_db

# Volume is calculated starting with max volume, but a volume of 0 should be
# the minimum value.
lines.reverse()
print('\n'.join(lines))
print('       0, // Off means (almost) no resistance -> no attenuation')
print(f'  // {num_chosen} values')
print(footer)
