const char ProductName[] = "ARDOP TNC";
const char ProductVersion[] = "3.0.1.18fm-BPQ";

// i revert 2500 levels add check for two tones
// j maintain AcquireFrameSyncRSBAvg state over samplee buffers

// Suspect tuning isn't accurate enough for psk frame type

// k add CorrectPhaseForTuningOffset for all types, including frame type
// iss sends break insted of ack at start
// speed up IDLE repeat (3 > 2 secs)

// l Try replying to IDLE with data (auto IRS/ISS switch without using BREAK)
// Now no need to send BREAK at start

// m Fix FEC
// fix repeat error if first frame after auto ISS swich id repeated

