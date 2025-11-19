#pragma once

// Halaman "SAVED"
const char saved[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
  <h2>SAVED</h2>
<script>
  setTimeout(() => {
    window.location.replace("/");
  }, 3000);
</script>
</body>
</html>
)rawliteral";

// Halaman "INDEX"
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
  <h2 style="text-align: center; color: #2c3e50;">ESP Web Server</h2>

  <div id="realtime" style="margin: 10px auto; padding: 10px; text-align: center; font-size: 18px;"></div>

  <div style="display: flex; justify-content: center; gap: 10px; margin-bottom: 20px;">
    <button id="btn-scan" style="padding: 10px 20px; border: none; background-color: #27ae60; color: white; border-radius: 5px; cursor: pointer;">Scan</button>
  </div>

  <div id="element-scan" style="margin: 10px auto; padding: 10px; text-align: center;"></div>

  <form action="/wifi" method="post" style="display: flex; flex-direction: column; padding: 20px; max-width: 400px; margin: auto; background-color: #f7f7f7; border-radius: 10px; box-shadow: 0 2px 5px rgba(0,0,0,0.1);">
    <label for="device" style="margin-bottom: 5px; font-weight: bold;">Device:</label>
    <select name="device" id="device" style="margin-bottom: 15px; padding: 8px; border-radius: 5px; border: 1px solid #ccc;">
      <option value="">NOT REGISTERED</option>
      <option value="BED">BED</option>
      <option value="TOILET">TOILET</option>
      <option value="LAMPP">LAMPP</option>
    </select>

    <input type="number" id="ip" name="ip" placeholder="STATIC IP" style="margin-bottom: 15px; padding: 10px; border-radius: 5px; border: 1px solid #ccc;">
    <input type="text" id="ssid" name="ssid" placeholder="SSID" style="margin-bottom: 15px; padding: 10px; border-radius: 5px; border: 1px solid #ccc;">
    <input type="text" name="id" id="id" placeholder="ID" style="margin-bottom: 15px; padding: 10px; border-radius: 5px; border: 1px solid #ccc;">

    <button type="submit" style="padding: 10px; background-color: #e67e22; color: white; border: none; border-radius: 5px; cursor: pointer;">Save</button>
  </form>

  <div style="text-align: center; margin-top: 20px;">
    <button id="restart" style="padding: 10px 20px; background-color: #c0392b; color: white; border: none; border-radius: 5px; cursor: pointer;">Restart</button>
  </div>

<script>
  function onStart() {
    const xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          const dataForm = JSON.parse(xhttp.responseText);
          document.getElementById("ssid").value = dataForm.ssid;
          document.getElementById("id").value = dataForm.id;
          document.getElementById("device").value = dataForm.device;
          document.getElementById("ip").value = dataForm.ip;
        }
    };
    xhttp.open("GET", "/get", true);
    xhttp.send();
  }

  onStart();

  document.getElementById("btn-scan").onclick = () => {
    const xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          const scanNetwork = JSON.parse(xhttp.responseText);
          let html = "";
          scanNetwork.forEach((val) => {
            html += `<a href='#' onclick="setSSID('${val.SSID}')">${val.SSID} : ${val.RSSI}</a><br>`
          })
          document.getElementById("element-scan").innerHTML = html;
        }
    };
    xhttp.open("GET", "/scan", true);
    xhttp.send();
  }

  document.getElementById("restart").onclick = () => {
    const xhttp = new XMLHttpRequest();
    xhttp.open("GET", "/restart", true);
    xhttp.send();
  }

  function setSSID(val) {
    document.getElementById("ssid").value = val;
  }
</script>
</body>
</html>
)rawliteral";
