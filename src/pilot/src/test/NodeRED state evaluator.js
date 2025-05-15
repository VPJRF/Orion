let currentState = flow.get("currentState") || "bed_in";
let data = msg.payload;
let nextState = currentState;

const now = Date.now();
const timeout = 5 * 60 * 1000; // 5 minutes in ms
let stateTimestamp = flow.get("stateTimestamp") || now;

// === Forced State Override ===
if (data.forceState) {
    let forced = data.forceState;
    node.warn(`Forced state to: ${forced}`);
    flow.set("currentState", forced);
    flow.set("stateTimestamp", now);
    flow.set("prevState", forced);

    // Compose dummy payload
    data = {device: "manual", state: forced, PIR: 0, IMU: 0, LED: 0, BOX: 0, FBK: 0};

    msg.payload = data;
    msg.measurement = "orion_data";
    return msg;
}

// Manual reset
if (msg.payload === "reset" || msg.topic === "reset") {
    nextState = "bed_in";
    data = {device: "bed", state: "bed_in", BOX: 0, FBK: 0, IMU: 0, LED: 0, PIR: 0};
}

// Check if stuck in state for too long
if (now - stateTimestamp > timeout) {
    node.warn(`State "${currentState}" timed out. Resetting to bed_in.`);
    nextState = "bed_in";
    stateTimestamp = now;
}

// === Simple Sequential State Machine ===
// 1) bed_in → bed_uit when bed-PIR=true AND at least 10s in bed_in
if (currentState === "bed_in") {
    let bedInTime = flow.get("bedInTime");
    if (bedInTime === undefined) {
        bedInTime = now;
        flow.set("bedInTime", now);
    }
    let minDelay = 10000; // 10 seconds
    if (
        data.device === "bed" &&
        data.PIR === true &&
        (now - bedInTime > minDelay)
    ) {
        nextState = "bed_uit";
    }
}
// 2) bed_uit → kamer_uit when kamer-IMU=true
else if (currentState === "bed_uit") {
    let bedUitTime = flow.get("bedUitTime");
    if (bedUitTime === undefined) {
        bedUitTime = now;
        flow.set("bedUitTime", now);
    }
    if (data.device === "kamer" && data.IMU === true) {
        nextState = "kamer_uit";
    }
    // Revert to bed_in if stuck in bed_uit for 20s without bed-PIR
    if (data.device === "bed" && data.PIR === true) {
        flow.set("bedUitTime", now); // reset timer if bed-PIR is triggered
    }
    if ((now - bedUitTime) > 20000) {
        nextState = "bed_in";
        node.warn("No bed-PIR for 20s in bed_uit: Resetting to bed_in");
        flow.set("bedUitTime", now); // reset timer
    }
}
// 3) kamer_uit → toilet_in when toilet-IMU=true
else if (currentState === "kamer_uit") {
    let toiletInTime = flow.get("toiletInTime");
    if (toiletInTime === undefined) {
        toiletInTime = now;
        flow.set("toiletInTime", now);
    }
    if (data.device === "toilet" && data.IMU === true) {
        nextState = "toilet_in";
        flow.set("toiletInTime", now); // Mark entry time to toilet_in
    }
}
// 4) toilet_in → toilet_uit when toilet-IMU=true AND at least 10s in toilet_in
else if (currentState === "toilet_in") {
    let toiletInTime = flow.get("toiletInTime");
    if (toiletInTime === undefined) {
        toiletInTime = now;
        flow.set("toiletInTime", now);
    }
    let minDelay = 10000; // 10 seconds
    if (
        data.device === "toilet" &&
        data.IMU === true &&
        (now - toiletInTime > minDelay)
    ) {
        nextState = "toilet_uit";
    }
}
// 5) toilet_uit → kamer_in when kamer-IMU=true
else if (currentState === "toilet_uit") {
    if (data.device === "kamer" && data.IMU === true) {
        nextState = "kamer_in";
    }
}
// 6) kamer_in → bed_in when bed-PIR=true
else if (currentState === "kamer_in") {
    if (data.device === "bed" && data.PIR === true) {
        nextState = "bed_in";
        flow.set("bedInTime", now); // Mark entry time to bed_in
    }
}

// === State Change ===
if (nextState !== currentState) {
    node.warn(`State changed to: ${nextState}`);
    flow.set("currentState", nextState);
    flow.set("stateTimestamp", now);
    flow.set("prevState", nextState); // <-- Add this line
    // Reset bedUitTime when leaving bed_uit state
    if (currentState === "bed_uit") {
        flow.set("bedUitTime", undefined);
    }
} else {
    flow.set("stateTimestamp", stateTimestamp);
}

// Attach state to payload for logging or DB
data.state = nextState;

// Convert boolean fields to integers for InfluxDB compatibility
["BOX", "FBK", "IMU", "LED", "PIR"].forEach(key => {
    if (typeof data[key] === "boolean") {
        data[key] = data[key] ? 1 : 0;
    }
});

msg.payload = data;
msg.measurement = "orion_data";

return msg;
