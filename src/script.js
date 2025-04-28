const SERVICE_UUID = '0000180c-0000-1000-8000-00805f9b34fb';
const SSID_CHAR_UUID = '00002a57-0000-1000-8000-00805f9b34fb';
const PASSWORD_CHAR_UUID = '00002a58-0000-1000-8000-00805f9b34fb';
let ssidCharacteristic;
let passwordCharacteristic;
let arduinoIP = null;
let logCharacteristic;

async function connectBLE() {
  const status = document.getElementById('status');
  try {
    status.textContent = 'Requesting Bluetooth device...';
    status.className = '';
    const device = await navigator.bluetooth.requestDevice({
      filters: [{ namePrefix: 'Arduino' }],
      optionalServices: [SERVICE_UUID]
    });

    status.textContent = 'Connecting...';
    status.className = '';
    const server = await device.gatt.connect();
    const service = await server.getPrimaryService(SERVICE_UUID);
    ssidCharacteristic = await service.getCharacteristic(SSID_CHAR_UUID);
    passwordCharacteristic = await service.getCharacteristic(PASSWORD_CHAR_UUID);
    logCharacteristic = await service.getCharacteristic('00002a59-0000-1000-8000-00805f9b34fb');
    await logCharacteristic.startNotifications();
    logCharacteristic.addEventListener('characteristicvaluechanged', event => {
      const value = new TextDecoder().decode(event.target.value);
      document.getElementById('logDisplay').textContent = value;
    });

    if (!status.textContent.includes('Wi-Fi connected')) {
      status.textContent = 'Connected to BLE device!';
      status.className = 'success';
    }

    device.addEventListener('gattserverdisconnected', () => {
      if (!status.textContent.includes('Wi-Fi connected')) {
        status.textContent = 'Disconnected from BLE device';
        status.className = 'warning';
      }
    });
  } catch (error) {
    status.textContent = 'Error: ' + error.message;
    status.className = 'error';
  }
}

async function connectWiFi() {
  const ssid = document.getElementById('ssid').value;
  const password = document.getElementById('password').value;
  const status = document.getElementById('status');

  if (!ssid || !password) {
    status.textContent = 'Please enter both SSID and password.';
    status.className = 'error';
    return;
  }

  if (!ssidCharacteristic) {
    status.textContent = 'Error: BLE not connected.';
    status.className = 'error';
    return;
  }

  status.textContent = 'Wi-Fi credentials sent, waiting for connection...';
  status.className = '';

  try {
    await ssidCharacteristic.writeValue(new TextEncoder().encode(ssid));
  } catch (error) {}

  try {
    await passwordCharacteristic.writeValue(new TextEncoder().encode(password));
  } catch (error) {}

  setTimeout(pollWiFiStatus, 4000);
}

async function changeScene() {
  updateArduinoIP();
  if (!arduinoIP) return;
  try {
    await fetch(`http://${arduinoIP}/change_scene`);
  } catch (error) {}
}

function saveWiFiCredentials() {
  const ssid = document.getElementById('ssid').value;
  const password = document.getElementById('password').value;
  if (!ssid || !password) return;
  localStorage.setItem('wifiSSID', ssid);
  localStorage.setItem('wifiPassword', password);
}

function restoreWiFiCredentials() {
  const ssid = localStorage.getItem('wifiSSID');
  const password = localStorage.getItem('wifiPassword');
  if (ssid) document.getElementById('ssid').value = ssid;
  if (password) document.getElementById('password').value = password;
}

function buttonClick(buttonId) {
  // Only highlight if the button has an id in the DOM
  const btn = document.querySelector(`#BLE-container #btn-${buttonId}, #wifi-container #btn-${buttonId}`);
  if (btn) {
    // Remove 'active' from all buttons with an id starting with 'btn-'
    document.querySelectorAll('#BLE-container button[id^="btn-"], #wifi-container button[id^="btn-"]').forEach(b => {
      b.classList.remove('active');
    });
    btn.classList.add('active');
  }
  if (!ssidCharacteristic || !passwordCharacteristic) return;
  try {
    ssidCharacteristic.writeValue(new TextEncoder().encode(buttonId));
  } catch (error) {}
}

function toggleWiFi() {
  document.getElementById('BLE-container').style.display = 'none';
  document.getElementById('wifi-container').style.display = 'block';
}

function toggleBLE() {
  document.getElementById('BLE-container').style.display = 'block';
  document.getElementById('wifi-container').style.display = 'none';
}

async function disconnectWiFiAndReconnectBLE() {
  const status = document.getElementById('status');
  toggleBLE();
  updateArduinoIP();
  if (!arduinoIP) {
    status.textContent = 'Arduino IP not available.';
    status.className = 'error';
    return;
  }
  try {
    const response = await fetch(`http://${arduinoIP}/disconnect_wifi`);
    if (response.ok) {
      status.textContent = 'Wi-Fi disconnected. Reconnecting to BLE...';
      status.className = 'warning';
    } else {
      throw new Error();
    }
  } catch (error) {
    status.textContent = 'Error disconnecting Wi-Fi.';
    status.className = 'error';
    return;
  }
  await new Promise((resolve) => setTimeout(resolve, 2000));
  try {
    const device = await navigator.bluetooth.requestDevice({
      filters: [{ namePrefix: 'Arduino' }],
      optionalServices: [SERVICE_UUID]
    });
    const server = await device.gatt.connect();
    const service = await server.getPrimaryService(SERVICE_UUID);
    ssidCharacteristic = await service.getCharacteristic(SSID_CHAR_UUID);
    passwordCharacteristic = await service.getCharacteristic(PASSWORD_CHAR_UUID);
    status.textContent = 'Reconnected to BLE.';
    status.className = 'success';
  } catch (error) {
    status.textContent = 'Error reconnecting to BLE.';
    status.className = 'error';
  }
}

function updateArduinoIP() {
  const ipInput = document.getElementById('arduino-ip');
  arduinoIP = ipInput.value.trim();
}

async function pollWiFiStatus() {
  updateArduinoIP();
  const status = document.getElementById('status');
  if (!arduinoIP) return;
  try {
    const response = await fetch(`http://${arduinoIP}/wifi_status`);
    if (response.ok) {
      const text = await response.text();
      if (text.includes('connected')) {
        status.textContent = 'Wi-Fi connected!';
        status.className = 'success';
      } else {
        status.textContent = 'Wi-Fi not connected.';
        status.className = 'error';
      }
    } else {
      status.textContent = 'Could not check Wi-Fi status.';
      status.className = 'error';
    }
  } catch (error) {
    status.textContent = 'Could not reach Arduino for Wi-Fi status.';
    status.className = 'error';
  }
}

async function fetchLog() {
  updateArduinoIP();
  if (!arduinoIP) return;
  try {
    const response = await fetch(`http://${arduinoIP}/get_log`);
    if (response.ok) {
      const text = await response.text();
      document.getElementById('logDisplay').textContent = text;
    }
  } catch (error) {
    console.error('Failed to fetch log:', error);
  }
}