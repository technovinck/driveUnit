// webpage.h
#ifndef WEBPAGE_H
#define WEBPAGE_H

const char *htmlHomePage = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<html>
  <head>
    <meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
    <style>
      .arrows {
        font-size: 70px;
        color: red;
      }
      .circularArrows {
        font-size: 80px;
        color: blue;
      }
      td {
        background-color: black;
        border-radius: 25%;
        box-shadow: 5px 5px #888888;
      }
      td:active {
        transform: translate(5px,5px);
        box-shadow: none; 
      }

      .noselect {
        -webkit-touch-callout: none; /* iOS Safari */
          -webkit-user-select: none; /* Safari */
          -khtml-user-select: none; /* Konqueror HTML */
            -moz-user-select: none; /* Firefox */
              -ms-user-select: none; /* Internet Explorer/Edge */
                  user-select: none; /* Non-prefixed version, currently
                                        supported by Chrome and Opera */
      }

      #toggleButton {
        background-color: green;
        color: white;
      }

      #toggleButton.active {
        background-color: red;
      }
    </style>
  </head>
  <body class="noselect" align="center" style="background-color:white">
     
    <h1 style="color: teal;text-align:center;">Muesam Mecanum Drive</h1>
    <h2 style="color: teal;text-align:center;">Wi-Fi &#128663; Control</h2>
    
    <table id="mainTable" style="width:400px;margin:auto;table-layout:fixed" CELLSPACING=10>
      <tr>
        <td><span class="arrows" ontouchstart="onTouchStart('5')" ontouchend="onTouchEnd('5')" onmousedown="onMouseDown('5')" onmouseup="onMouseUp('5')">&#11017;</span></td>
        <td><span class="arrows" ontouchstart="onTouchStart('1')" ontouchend="onTouchEnd('1')" onmousedown="onMouseDown('1')" onmouseup="onMouseUp('1')">&#8679;</span></td>
        <td><span class="arrows" ontouchstart="onTouchStart('6')" ontouchend="onTouchEnd('6')" onmousedown="onMouseDown('6')" onmouseup="onMouseUp('6')">&#11016;</span></td>
      </tr>
      
      <tr>
        <td><span class="arrows" ontouchstart="onTouchStart('3')" ontouchend="onTouchEnd('3')" onmousedown="onMouseDown('3')" onmouseup="onMouseUp('3')">&#8678;</span></td>
        <td></td>    
        <td><span class="arrows" ontouchstart="onTouchStart('4')" ontouchend="onTouchEnd('4')" onmousedown="onMouseDown('4')" onmouseup="onMouseUp('4')">&#8680;</span></td>
      </tr>
      
      <tr>
        <td><span class="arrows" ontouchstart="onTouchStart('7')" ontouchend="onTouchEnd('7')" onmousedown="onMouseDown('7')" onmouseup="onMouseUp('7')">&#11019;</span></td>
        <td><span class="arrows" ontouchstart="onTouchStart('2')" ontouchend="onTouchEnd('2')" onmousedown="onMouseDown('2')" onmouseup="onMouseUp('2')">&#8681;</span></td>
        <td><span class="arrows" ontouchstart="onTouchStart('8')" ontouchend="onTouchEnd('8')" onmousedown="onMouseDown('8')" onmouseup="onMouseUp('8')">&#11018;</span></td>
      </tr>
    
      <tr>
        <td><span class="circularArrows" ontouchstart="onTouchStart('9')" ontouchend="onTouchEnd('9')" onmousedown="onMouseDown('9')" onmouseup="onMouseUp('9')">&#8634;</span></td>
        <td style="background-color:white;box-shadow:none"></td>
        <td><span class="circularArrows" ontouchstart="onTouchStart('10')" ontouchend="onTouchEnd('10')" onmousedown="onMouseDown('10')" onmouseup="onMouseUp('10')">&#8635;</span></td>
      </tr>
      <tr>
        <td colspan="3">
          <!-- Geschwindigkeitsregler -->
          <input type="range" id="speedSlider" min="0" max="255" value="128" oninput="updateSpeed(this.value)" style="width: 100%;">
        </td>
      </tr>
      <tr>
        <td colspan="3">
          <!-- Einzelradansteuerung aktivieren -->
          <button id="toggleButton" onclick="toggleIndividualControl()" ontouchstart="toggleIndividualControl()">NormalMode</button>
        </td>
      </tr>
      <tr>
        <td colspan="3">
          <!-- Statusanzeige -->
          <p id="statusDisplay">Status: NormalMode</p>
        </td>
      </tr>
    </table>

<script>
  var isIndividualControlActive = false; // Globale Variable zur Verfolgung des Status des Buttons
  var webSocketUrl = "ws:\/\/" + window.location.hostname + "/ws";
  var websocket;
  
  function initWebSocket() 
  {
    websocket = new WebSocket(webSocketUrl);
    websocket.onopen    = function(event){};
    websocket.onclose   = function(event){setTimeout(initWebSocket, 2000);};
    websocket.onmessage = function(event){};
  }

  function onTouchStart(value) 
  {
    if (isIndividualControlActive) {
      value = parseInt(value) + 20;
    }
    websocket.send(value);
  }

  function onTouchEnd(value) 
  {
    if (isIndividualControlActive) {
      value = parseInt(value) + 20;
    }
    websocket.send("0");
  }

  function onMouseDown(value) 
  {
    if (isIndividualControlActive) {
      value = parseInt(value) + 20;
    }
    websocket.send(value);
  }

  function onMouseUp(value) 
  {
    if (isIndividualControlActive) {
      value = parseInt(value) + 20;
    }
    websocket.send("0");
  }
      
  window.onload = initWebSocket;
  document.getElementById("mainTable").addEventListener("touchend", function(event){
    event.preventDefault()
  });      
  
  function updateSpeed(speed) 
  {
    websocket.send("SPEED:" + speed);
  }

  function toggleIndividualControl() 
  {
    isIndividualControlActive = !isIndividualControlActive; // Umschalten des Status
    updateStatusDisplay();
    
    var button = document.getElementById("toggleButton");
    if (isIndividualControlActive) {
        button.innerHTML = "TestMode"; // Ändern Sie den Text
        button.style.color = "red"; // Ändern Sie die Schriftfarbe
        websocket.send("INDIVIDUAL_CONTROL:ON");
    } else {
        button.innerHTML = "NormalMode"; // Ändern Sie den Text zurück
        button.style.color = "black"; // Ändern Sie die Schriftfarbe zurück
        websocket.send("INDIVIDUAL_CONTROL:OFF");
    }
  }

  function updateStatusDisplay() 
  {
    var statusDisplay = document.getElementById("statusDisplay");
    if (isIndividualControlActive) {
        statusDisplay.textContent = "Status: TestMode";
        statusDisplay.style.color = "red"; // Ändern Sie die Schriftfarbe, wenn gewünscht
    } else {
        statusDisplay.textContent = "Status: NormalMode";
        statusDisplay.style.color = "black"; // Ändern Sie die Schriftfarbe, wenn gewünscht
    }
  }

  function onButtonClick(value) 
  {
    if (isIndividualControlActive) {
      value = parseInt(value) + 10;
    }
    websocket.send(value);
    updateStatusDisplay(); // Status bei Klick aktualisieren
  }

</script>

    
  </body>
</html>


)HTMLHOMEPAGE";

#endif // WEBPAGE_H