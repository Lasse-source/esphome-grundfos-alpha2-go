static std::vector<uint8_t> buf;
buf.insert(buf.end(), x.begin(), x.end());
if (buf.size() > 300) buf.clear();
while (buf.size() >= 2) {
  if (buf[0] != 0x24) { buf.erase(buf.begin()); continue; }
  size_t tot = (size_t) buf[1] + 4;
  if (tot < 15 || tot > 120) { buf.erase(buf.begin()); continue; }
  if (buf.size() < tot) break;
  std::vector<uint8_t> f(buf.begin(), buf.begin() + tot);
  buf.erase(buf.begin(), buf.begin() + tot);
  uint16_t crc = 0xFFFF;
  for (size_t i = 1; i + 2 < tot; i++) {
    crc ^= (uint16_t) (f[i] << 8);
    for (int k = 0; k < 8; k++)
      crc = (crc & 0x8000) ? (uint16_t) ((crc << 1) ^ 0x1021) : (uint16_t) (crc << 1);
  }
  crc = (uint16_t) ~crc;
  uint16_t rx = (uint16_t) ((f[tot - 2] << 8) | f[tot - 1]);
  if (crc != rx) {
    ESP_LOGW("geni", "bad checksum, frame dropped");
    continue;
  }
  auto u32 = [&](size_t off) -> uint32_t {
    return ((uint32_t) f[13 + off] << 24) | ((uint32_t) f[14 + off] << 16) |
           ((uint32_t) f[15 + off] << 8) | (uint32_t) f[16 + off];
  };
  auto flt = [&](size_t off) -> float {
    uint32_t u = u32(off);
    float v;
    memcpy(&v, &u, 4);
    return v;
  };
  static const uint8_t T_POWER[8] = {0x30, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00, 0x29};
  static const uint8_t T_FLOW[8] = {0x2b, 0x00, 0x02, 0x35, 0x02, 0x00, 0x00, 0x24};
  static const uint8_t T_COUNT[8] = {0x23, 0x00, 0x00, 0xf8, 0x02, 0x00, 0x00, 0x1c};
  size_t plen = tot - 15;
  if (memcmp(&f[5], T_POWER, 8) == 0 && plen >= 24) {
    float v = flt(0), a = flt(8), p = flt(12), n = flt(20);
    if (std::isfinite(v)) id(grundfos_voltage).publish_state(v);
    if (std::isfinite(a)) id(grundfos_current).publish_state(a);
    if (std::isfinite(p)) id(grundfos_power).publish_state(p);
    if (std::isfinite(n)) id(grundfos_speed).publish_state(n);
  } else if (memcmp(&f[5], T_FLOW, 8) == 0 && plen >= 8) {
    float q = flt(0), h = flt(4);
    if (std::isfinite(q)) id(grundfos_flow).publish_state(q * 3600.0f);
    if (std::isfinite(h)) id(grundfos_head).publish_state(h * 0.0001f);
  } else if (memcmp(&f[5], T_COUNT, 8) == 0 && plen >= 12) {
    id(grundfos_starts).publish_state((float) u32(0));
    id(grundfos_runtime).publish_state((float) u32(8) / 3600.0f);
  } else {
    ESP_LOGD("geni", "unknown response type, frame skipped");
  }
}
return NAN;
