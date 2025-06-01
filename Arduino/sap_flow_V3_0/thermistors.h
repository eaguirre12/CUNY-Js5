#pragma once

#include <cstdint>

bool setup_thermistors();

int16_t read_thermistor(int mux_channel);

enum class MuxChannel
{
  NEAR_INNER = 0,
  NEAR_MIDDLE = 5,
  NEAR_OUTER = 3,
  FAR_INNER = 2,
  FAR_MIDDLE = 1,
  FAR_OUTER = 4,
  SPARE_6 = 6,
  SPARE_7 = 7,
};

float readThermistorTemp(MuxChannel channel);
