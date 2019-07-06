#define ProductName "ARDOP TNC"
#define ProductVersion "3.0.1.18fk-BPQ"

// i revert 2500 levels add check for two tones
// j maintain AcquireFrameSyncRSBAvg state over samplee buffers

// Suspect tuning isn't accurate enough for psk frame type
// k add CorrectPhaseForTuningOffset for all types, including frame type
// iss sends break insted of ack at start
// speed up IDLE repeat (3 > 2 secs)
