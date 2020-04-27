#ifndef HTML_H
#define HTML_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {
     font-family: Arial;
     font-size: 3.0rem;
    }
    .label {
      font-style: italic;
    }
    #error {
      background-color: red;
    }
    .activity {
      font-size: 0.5rem;
      margin: 0.4rem;
    }
  </style>
</head>
<body>
  <h3>Volume Control</h3>
  <div>
    <input type="range" min="0" max="%NUM_STEPS%" id="volume" name="volume" oninput="setVolume(this.value)">
    <label for="volume">Volume</label>
  </div>
  <div id="error" style="display: none;">
    Error Connecting
  </div>
</body>

<script>
const getDelay = 5 * 1000;
let getTimer = setInterval(getVolume, getDelay);
setTimer(getVolume, 0);

function showError() {
  document.getElementById("error").style.display = "block";
}

function setVolume(volume) {
  if (getTimer > 0) {
    clearInterval(getTimer);
    getTimer = -1;
  }

  var xhr = new XMLHttpRequest();
  xhr.timeout = 10 * 1000;
  xhr.onreadystatechange = function() { // Call a function when the state changes.
    if (this.readyState === XMLHttpRequest.DONE && this.status === 200) {
      document.getElementById("error").style.display = "none";
    }
  }
  xhr.onerror = showError;
  xhr.onabort = showError;
  xhr.open("POST", "/volume", true);
  xhr.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
  xhr.send("volume=" + volume);

  getTimer = setInterval(getVolume, getDelay);
}

function getVolume() {
  var xhr = new XMLHttpRequest();
  xhr.timeout = 10 * 1000;
  xhr.onload = function() {
    document.getElementById("volume").value  = this.responseText;
    document.getElementById("error").style.display = "none";
  };
  xhr.onerror = showError;
  xhr.onabort = showError;
  xhr.open("GET", "/volume", true);
  xhr.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
  xhr.send();
}
</script>
</html>)rawliteral";

#endif
