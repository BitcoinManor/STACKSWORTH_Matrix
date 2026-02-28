# STACKSWORTH Matrix - Testing Log

## Purpose
Track stability testing, WiFi reliability issues, and production readiness validation.

---

## Feb 26, 2026 - WiFi Stability Comparison Test

### Objective
Determine if home WiFi disconnection issues are router-specific or firmware bugs.

### Test Setup
- **Unit 1 (Control):** Connected to mobile hotspot
- **Unit 2 (Diagnosis):** Connected to home WiFi network
- **Firmware Version:** v2.0.50
- **Test Duration:** Overnight (~12-16 hours)
- **Monitoring:** Serial logging on Unit 1 (hotspot)

### Hypothesis
Home router may be:
- Dropping DHCP leases prematurely
- Using aggressive WiFi power-saving modes
- Incompatible with ESP32 keep-alive behavior
- Enforcing client isolation or security policies

Mobile hotspot should remain stable, proving firmware WiFi code is functional.

### Variables Being Tested
| Variable | Unit 1 (Hotspot) | Unit 2 (Home WiFi) |
|----------|------------------|-------------------|
| Network Type | Mobile hotspot | Home router |
| Expected Stability | High | Unknown |
| Serial Logging | Yes | No (IDE limitation) |
| Baseline | Control group | Test subject |

### Success Criteria
- ✅ Unit 1 runs 12+ hours without falling to AP mode
- ✅ Serial log shows no WDT crashes
- ✅ Serial log shows stable WiFi connection
- ⚠️ Unit 2 behavior determines next steps

### Expected Outcomes

**Scenario A: Both Units Stable**
- Firmware is production-ready
- Previous home WiFi issues were transient
- Proceed with shipping preparation

**Scenario B: Unit 1 Stable, Unit 2 Fails**
- Router compatibility issue confirmed
- Add diagnostic logging for WiFi disconnect reasons
- Consider static IP option for problematic routers
- Test on additional router brands

**Scenario C: Both Units Fail**
- Firmware bug in v2.0.50 (likely lwIP/HTTPUpdate conflict)
- Revert to v2.0.47
- Re-implement OTA without HTTPUpdate library
- Delay shipping until stable

### Results
*To be filled Feb 27, 2026 morning*

**Unit 1 (Hotspot):**
- Start Time: 00:46:50.409_________
- End Time: 06:34:04.280_________
- Status: _________
- Crashes: _________
- Notes: _________

**Unit 2 (Home WiFi):**
- Start Time: _________
- End Time: _________
- Status: _________
- Display State: _________
- Notes: _________

### Serial Log Analysis
*Attach or reference serial log file here*

### Next Steps
*To be determined based on results*

---

## Feb 23-24, 2026 - Initial v2.0.453 Overnight Test

### Results Summary
- **Runtime:** ~12 hours (18:51 PM - 06:36 AM)
- **Crashes:** 20 WDT timeouts
- **Root Cause:** Sequential API calls (7 calls) exceeded 12s watchdog timer
- **SatoNak Issues:** 59 HTTP 429 rate limit errors (upstream CoinGecko)

### Actions Taken
- ✅ Added `esp_task_wdt_reset()` between each API call (v2.0.47)
- ✅ Identified SatoNak server needs caching improvements
- ⏳ WDT fix validation pending

---

## Feb 24-25, 2026 - v2.0.50 OTA Test (FAILED)

### Results Summary
- **Runtime:** ~2.5 hours before crash loop
- **Crashes:** 7+ crashes (1 WDT, 5+ lwIP, 1+ WiFi failure)
- **Root Cause:** HTTPUpdate library conflicts with AsyncWebServer
- **Error:** `assert failed: sys_untimeout` (lwIP threading violation)

### Actions Taken
- ❌ v2.0.50 marked unstable - DO NOT SHIP
- ⏳ OTA implementation needs rewrite (use Update.h directly)
- ⏳ Revert to v2.0.47 or fix threading issue

### Lessons Learned
- HTTPUpdate library incompatible with async web servers
- Always test new libraries overnight before production
- OTA is complex - needs dedicated testing phase

---

## Known Issues

### Critical
- [ ] v2.0.50: lwIP crash loop with HTTPUpdate + AsyncWebServer
- [ ] Home WiFi: Intermittent disconnections (router-specific?)

### Under Investigation
- [ ] WDT crashes: Fixed in v2.0.47? (validation pending)
- [ ] SatoNak 429 errors: Server-side issue, graceful handling works

### Requires Testing
- [ ] v2.0.47 overnight stability (WDT fix validation)
- [ ] Multiple router brand compatibility
- [ ] OTA update mechanism (needs reimplementation)

---

## Production Readiness Checklist

### Stability
- [ ] 24-hour crash-free operation
- [ ] Multi-router WiFi compatibility validated
- [ ] WDT timeout issues resolved
- [ ] Memory leak testing (72+ hour run)

### Features
- [x] Device identification (nickname, MAC ID, identify button)
- [x] All data displays functional
- [x] Portal configuration working
- [x] mDNS (Matrix.local) functional
- [ ] OTA updates (blocked on stability)

### Polish
- [ ] WiFi diagnostic logging
- [ ] Static IP option (for problematic routers)
- [ ] IP address display screen (troubleshooting aid)
- [ ] Router compatibility documentation

### Documentation
- [ ] User setup guide
- [ ] Troubleshooting guide (WiFi issues)
- [ ] Known router compatibility list
- [ ] Developer handoff documentation

---

## Test Equipment

### Hardware
- ESP32 DevKit boards (multiple units)
- MD_Parola LED Matrix (16 modules, 2 zones)
- USB cables, power supplies
- Multiple test routers (if available)

### Networks
- Home WiFi router: [Model/Brand needed]
- Mobile hotspot: [Phone model/carrier]
- Office WiFi: [If available]
- Friend's network: [For compatibility testing]

### Software
- Arduino IDE 2.x
- Serial monitor / PuTTY
- GitHub for version control
- Web Flasher (for production)

---

## Notes & Observations

- ESP32 requires timing trick for sketch upload when program is running (NORMAL)
- Mobile hotspot has client isolation - Matrix.local won't work (EXPECTED)
- Serial monitor can only monitor one unit at a time (IDE limitation)
- v2.0.50 lwIP crashes correlate with initial data fetch timing
- Home router issues appeared after adding more features (coincidence or related?)

---

## Version History Reference

- **v2.0.45:** Original stable baseline
- **v2.0.451:** Display cycle safety + 30s WiFi timeout
- **v2.0.452:** WDT crash prevention (initial fetch)
- **v2.0.453:** WiFi resilience (1hr grace period)
- **v2.0.4535:** Heap constant refactoring
- **v2.0.46:** Device identification system
- **v2.0.47:** WDT fix (sequential API calls) + mDNS simplification
- **v2.0.50:** OTA updates (UNSTABLE - DO NOT USE)
