#pragma once
#include <cstdint>

const uint8_t DELIM = 0x0D;

const uint8_t DATA_GET_REG = 0x80;
const uint8_t VERSION_GET_REG = 0x72;

const uint8_t MODE_SET_REG = 0x03;
const uint8_t MODE_GET_REG = 0x04;
const uint8_t MODE_ARG_AUTO = 0x00;
const uint8_t MODE_ARG_MANUAL = 0x01;
const uint8_t MODE_ARG_CONFIG = 0x02;
// there could also be another "burst"-like mode here
// where the "response" format includes multiple
// data fields
