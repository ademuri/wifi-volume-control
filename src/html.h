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
    #error {
      background-color: red;
    }
    .activity {
      font-size: 0.5rem;
      margin: 0.4rem;
    }
    .container {
      width: 100%%;
      max-width: 600px;
    }
    .container div {
      margin: 1em 0em;
    }
    .volume-button {
      font-size: 2em;
      padding: 0.2em 0.4em;
    }
    #volume {
      width: 100%%;
    }
  </style>
</head>
<body>
  <h3>Volume Control</h3>
  <div class="container">
    <div>
      <input type="range" min="0" max="%NUM_STEPS%" id="volume" name="volume" oninput="setVolume(this.value)">
    </div>
    <div>
      <input type="button" value="-" class="volume-button" style="float: left;" onclick="changeVolume(-1)">
      <input type="button" value="+" class="volume-button" style="float: right;" onclick="changeVolume(1)">
    </div>
    <div id="error" style="display: none;">
      Error Connecting
    </div>
  </div>
</body>

<script>
const getDelay = 5 * 1000;
let getTimer = setInterval(getVolume, getDelay);
setTimeout(getVolume, 0);

function showError() {
  document.getElementById("error").style.display = "block";
}

function changeVolume(change) {
  setVolume(change + parseInt(document.getElementById("volume").value));
}

function setVolume(volume) {
  if (getTimer > 0) {
    clearInterval(getTimer);
    getTimer = -1;
  }

  document.getElementById("volume").value = volume;
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
