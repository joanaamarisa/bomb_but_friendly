#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>


// ------------------- WIFI CONFIG -------------------
const char* ssid = "Bomb_but_its_friendly";
const char* password = "aac2026x";

ESP8266WebServer server(80);

// buffer to store last message from serial
String lastStatus = "";

// ------------------- HTML INTERFACE -------------------
const char index_html[] PROGMEM = R"rawquote(

<!DOCTYPE html>
<html lang="pt">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Defuse the Bomb</title>
    <style>

        :root {
            --main-bg: #121212;
            --panel-bg: #262626;
            --warning-text: #ffd700;
        }


        body {
            background-color: var(--main-bg);
            color: #dcdcdc;
            font-family: 'Consolas', 'Monaco', monospace;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            min-height: 100vh;
            margin: 0;
            padding: 20px;
        }


        .bg-wires-container {
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            z-index: -1;
            opacity: 0.3;
        }


        @keyframes alarm {
            0% { box-shadow: inset 0 0 0px #2b0000; }
            50% { box-shadow: inset 0 0 80px #ff0000; }
            100% { box-shadow: inset 0 0 0px #2b0000; }
        }


        .alarm-active {
            animation: alarm 0.8s infinite;
        }


        #login-panel {
            background: #000;
            border: 2px solid #0f0;
            padding: 20px;
            text-align: center;
            width: 90%;
            max-width: 800px;
            margin-bottom: 20px;
            border-radius: 8px;
        }


        .player-input {
            background: #111;
            color: #fff;
            border: 1px solid #555;
            padding: 10px;
            width: 180px;
            text-transform: uppercase;
            margin: 5px;
        }


        .btn-generate {
            background: #004d00;
            color: #0f0;
            border: 1px solid #0f0;
            padding: 10px 20px;
            font-weight: bold;
            cursor: pointer;
        }


        .bomb-container {
            display: flex;
            background: var(--panel-bg);
            padding: 40px;
            border: 8px solid #333;
            position: relative;
            gap: 30px;
            width: 95%;
            max-width: 1200px;
            border-radius: 12px;
            justify-content: center;
            align-items: flex-start;
        }


        .wires-top {
            position: absolute;
            top: -20px;
            left: 50%;
            transform: translateX(-50%);
            width: 80%;
            display: flex;
            justify-content: space-around;
        }


        .wire-d {
            width: 6px;
            height: 35px;
            border-radius: 3px;
        }


        .column-wrapper {
            display: flex;
            flex-direction: column;
            width: 250px;
            align-items: center;
        }


        .clue-box {
            background: #111;
            border: 2px dashed #555;
            width: 100%;
            height: 90px;
            display: flex;
            align-items: center;
            justify-content: center;
            cursor: pointer;
            margin-bottom: 12px;
            padding: 10px;
            text-align: center;
            border-radius: 6px;
        }


        .clue-box.revealed {
            border: 2px solid #0f0;
            background: #001a00;
            color: var(--warning-text);
        }


        .clue-text {
            display: none;
            font-size: 0.85em;
            font-weight: bold;
            line-height: 1.3;
        }


        .clue-number {
            font-size: 2.2em;
            color: #444;
            font-weight: bold;
        }


        .timer-container {
            text-align: center;
            background: #000;
            padding: 15px;
            border: 5px solid #333;
            border-radius: 8px;
            width: 280px;
            min-height: 100px;
            display: flex;
            align-items: center;
            justify-content: center;
            margin-bottom: 20px;
        }


        #timer {
            font-family: 'Impact', sans-serif;
            font-size: 4em;
            color: red;
            text-shadow: 0 0 10px red;
            letter-spacing: 5px;
        }


        .explosion-overlay {
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: red;
            display: none;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            z-index: 100;
        }

    </style>

</head>

<body>

    <div class="bg-wires-container">

        <svg width="100%" height="100%">
            <path d="M-10,100 Q400,500 800,100 T1200,600" stroke="#222" stroke-width="5" fill="none"/>
            <path d="M-10,600 Q200,100 600,600 T1200,100" stroke="#181818" stroke-width="8" fill="none"/>
        </svg>

    </div>

   
    <div id="explosion" class="explosion-overlay">

        <h1 style="font-size:8em; color:black;">BOOM!</h1>
        <button onclick="remoteReset()" style="padding:15px; font-weight:bold; cursor:pointer;">RESTART SYSTEM</button>
    
    </div>


    <div id="login-panel">

        <h3 style="color:#0f0; margin-top:0;">AGENT IDENTIFICATION</h3>
        <input type="text" id="agentName" class="player-input" placeholder="NAME">
        <input type="number" id="studentNumber" class="player-input" placeholder="STUDENT ID">
        <button class="btn-generate" onclick="generateCodeMission()">GENERATE MISSION</button>
        <div id="code-display-info" style="color:#0f0; margin-top:10px; display:none; font-size:0.9em;"></div>
    
    </div>


    <div class="bomb-container">

        <div class="wires-top">
            <div class="wire-d" style="background:#8b0000"></div>
            <div class="wire-d" style="background:#00008b"></div>
            <div class="wire-d" style="background:#006400"></div>
            <div class="wire-d" style="background:#8b0000"></div>
            <div class="wire-d" style="background:#00008b"></div>
        </div>


        <div class="column-wrapper">
            <div style="color:var(--warning-text); margin-bottom:15px; font-weight:bold;">CRACK THE CODE</div>

            <div class="clue-box" onclick="revealClue(this)">
                <span class="clue-number">?</span>
                <span class="clue-text" id="clue1-text"></span>
            </div>

            <div class="clue-box" onclick="revealClue(this)">
                <span class="clue-number">?</span>
                <span class="clue-text" id="clue2-text"></span>
            </div>

            <div class="clue-box" onclick="revealClue(this)">
                <span class="clue-number">?</span>
                <span class="clue-text" id="clue3-text"></span>
            </div>

            <div class="clue-box" onclick="revealClue(this)">
                <span class="clue-number">?</span>
                <span class="clue-text" id="clue4-text"></span>
            </div>
        </div>


        <div class="column-wrapper" style="width:320px;">
            <div class="timer-container">
                <div id="timer">02:00</div>
            </div>

            <div style="color:var(--warning-text); font-weight:bold; margin-bottom:10px;">SIMON SAYS</div>

            <div class="clue-box" onclick="revealClue(this)" style="height:120px;">
                <span class="clue-number">?</span>
                <span class="clue-text">MEMORY PROTOCOL:<br>Repeat the sequence.</span>
            </div>
        </div>


        <div class="column-wrapper">
            <div style="color:var(--warning-text); margin-bottom:15px; font-weight:bold;">CUT THE WIRE</div>

            <div class="clue-box" onclick="revealClue(this)">
                <span class="clue-number">?</span>
                <span class="clue-text">RULE ALPHA:<br>EVEN Time = RED.</span>
            </div>

            <div class="clue-box" onclick="revealClue(this)">
                <span class="clue-number">?</span>
                <span class="clue-text">RULE BETA:<br>ODD Time = BLUE.</span>
            </div>
        </div>

    </div>


    <script>

        const audioCtx = new (window.AudioContext || window.webkitAudioContext)();
        let timeLeft = 120;
        let isRunning = false;
        let timerInterval;

        function playBeep(f = 800, d = 0.1) {

            try {
                const o = audioCtx.createOscillator();
                const g = audioCtx.createGain();
                o.type = 'square';
                o.frequency.setValueAtTime(f, audioCtx.currentTime);
                g.gain.setValueAtTime(0.05, audioCtx.currentTime);
                o.connect(g);
                g.connect(audioCtx.destination);
                o.start();
                o.stop(audioCtx.currentTime + d);
            }
            catch(e) { }
        }


        function remoteReset() {

            fetch('/resetHardware').then(() => {
                location.reload();
            });
        }


        function startTimer(duration) {

            if (isRunning) return;

            timeLeft = duration;
            isRunning = true;

            // resets timer text colour
            const timerEl = document.getElementById('timer');
            timerEl.style.color = 'red';
            timerEl.style.textShadow = '0 0 10px red';
            timerEl.style.fontSize = '4em';

            document.body.classList.add('alarm-active');

            if(timerInterval) clearInterval(timerInterval);

            timerInterval = setInterval(() => {

                if (!isRunning) return;

                timeLeft--;
                playBeep(1000, 0.05);
                const m = Math.floor(timeLeft / 60).toString().padStart(2,'0');
                const s = (timeLeft % 60).toString().padStart(2,'0');
                document.getElementById('timer').innerText = `${m}:${s}`;

                if (timeLeft <= 0) {
                    stopTimer(true);
                }

            }, 1000);
        }


        function updateTimerDisplay() {

            const m = Math.floor(timeLeft / 60).toString().padStart(2, '0');
            const s = (timeLeft % 60).toString().padStart(2, '0');
            document.getElementById('timer').innerText = `${m}:${s}`;

        }


        function stopTimer(exploded = false) {

            isRunning = false;
            clearInterval(timerInterval);
            document.body.classList.remove('alarm-active');
            const timerEl = document.getElementById('timer');

            if (exploded) {
                document.getElementById('explosion').style.display = 'flex';
                playBeep(200, 1);
            } else {
                timerEl.style.color = '#0f0';
                timerEl.style.fontSize = '2.8em'; 
                timerEl.style.letterSpacing = '2px';
                timerEl.innerText = "DEFUSED";
            }

        }


        function processExplosion() {

            timeLeft = 0;
            updateTimerDisplay();
            stopTimer(true);
            fetch('/triggerExplosion');

        }


        function generateCodeMission() {

            fetch('/resetHardware');

            const n = document.getElementById('studentNumber').value;
            if (n.length !== 8) return alert("Insert 8 digits");
            if (audioCtx.state === 'suspended') audioCtx.resume();

            const l4s = n.slice(-4);
            const l4 = parseInt(l4s);
            let r = (l4 % 2 === 0) ? Math.floor(l4 * 2) : Math.floor(l4 / 2);
            let c = r.toString().padStart(4, '0').substring(0, 4);

            document.getElementById('clue1-text').innerText = `BASE: ${l4s}`;
            document.getElementById('clue2-text').innerText = `PROTOCOL: ${l4%2===0 ? 'x2' : '/2'}`;
            document.getElementById('clue3-text').innerText = `VERIFICATION: ${c.substring(0, 2)}XX`;
            document.getElementById('clue4-text').innerText = `CODE: ${c}`;
            document.getElementById('code-display-info').style.display = 'block';
            startTimer(120);

        }


        function revealClue(el) {

            el.classList.add('revealed');
            el.querySelector('.clue-number').style.display = 'none';
            el.querySelector('.clue-text').style.display = 'block';

        }


        setInterval(() => {
            fetch('/getStatus').then(r => r.text()).then(data => {
            const msg = data.trim();
        
            if (msg.includes("START")) {
                startTimer(120);
            }
        
            if (msg.includes("WIN")) {
                isRunning = false; // stops timer loop
                stopTimer(false);  // DEFUSED
            }
        
            if (msg.includes("PENALTY")) {
                let pValue = parseInt(msg.split(":")[1]);
                if (!isNaN(pValue)) {
                    timeLeft -= pValue;
                    if (timeLeft <= 0) {
                        timeLeft = 0;
                        stopTimer(true); // if penalty reaches 0, explodes
                    }
                    updateTimerDisplay();
                    playBeep(400, 0.3);
                }
            }
        });
    }, 500);

    </script>
</body>
</html>
)rawquote";

int timeType = -1; // -1: none, 0: even, 1: odd

void setup() {
    
    Serial.begin(115200); 
    
    // moves arduino's TX to D7 
    Serial.swap(); 

    WiFi.softAP(ssid, password);
    
    server.on("/", []() { 
        server.send(200, "text/html", index_html); 
    });

    server.on("/getStatus", []() {
        server.send(200, "text/plain", lastStatus);
        if (lastStatus != "") {
            lastStatus = "";
        }
    });

    server.on("/sendTimeType", []() {
        timeType = server.arg("val").toInt();
        server.send(200);
    });

    server.on("/getTimeType", []() {
        server.send(200, "text/plain", String(timeType));
        timeType = -1; 
    });

    server.on("/resetHardware", []() {
    Serial.println("RESET"); 
    server.send(200, "text/plain", "Reset signal sent");
});

    server.begin();
}

void loop() {

    server.handleClient();

    // serial.available reads from D7
    if (Serial.available() > 0) {
        String msg = Serial.readStringUntil('\n');
        msg.trim();
        if (msg.length() > 0) {
            lastStatus = msg;
        }
    }
}