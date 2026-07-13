const CHANNEL_LABELS = [
    ['Vth1', 'Vth2', 'VMon1', 'VMon2'],
    ['Vth3', 'Vth4', 'VMon3', 'VMon4']
];

let statusInterval = null;

document.addEventListener('DOMContentLoaded', function() {
    const page = document.body.dataset.page;
    if (page === 'dashboard' || page === 'feb') {
        fetchStatus();
        statusInterval = setInterval(fetchStatus, 5000);
    }
});

document.addEventListener('focusin', function(e) {
    if (e.target.tagName === 'INPUT' && statusInterval) {
        clearInterval(statusInterval);
        statusInterval = null;
    }
});

document.addEventListener('focusout', function(e) {
    if (e.target.tagName === 'INPUT' && !statusInterval) {
        statusInterval = setInterval(fetchStatus, 5000);
    }
});

function fetchStatus() {
    fetch('/api/status')
        .then(r => r.json())
        .then(data => {
            const page = document.body.dataset.page;
            if (page === 'dashboard') updateDashboard(data);
            else if (page === 'feb') updateFebPage(data);
            updateWifiInfo(data);
        })
        .catch(err => console.error('fetch error:', err));
}

function updateWifiInfo(data) {
    const el = document.getElementById('wifi-info');
    if (el && data.wifi) {
        el.textContent = data.wifi.ip + ' | ' + data.wifi.ssid;
    }
}

function updateDashboard(data) {
    const container = document.getElementById('febs-container');
    if (!container) return;
    data.febs.forEach(feb => {
        let card = document.getElementById('feb-card-' + feb.id);
        if (!card) {
            card = createFebCard(feb);
            container.appendChild(card);
        } else {
            refreshFebCard(feb, card);
        }
    });
}

function createFebCard(feb) {
    const card = document.createElement('div');
    card.className = 'feb-card';
    card.id = 'feb-card-' + feb.id;

    const label = 'ABCD'[feb.id];
    const pcfStr = feb.pcf.toString(2).padStart(8, '0');

    let html = '<h2>FEB ' + label + ' <span class="pcf-badge" id="pcf-' + feb.id + '">PCF: ' + pcfStr + '</span></h2>';

    html += '<div class="section-title">Temperature</div>';
    html += '<div class="temp-display">Chip 0: <span id="temp-' + feb.id + '-0">' + feb.temp[0].toFixed(1) + '</span> &deg;C &nbsp;|&nbsp; Chip 1: <span id="temp-' + feb.id + '-1">' + feb.temp[1].toFixed(1) + '</span> &deg;C</div>';

    html += '<div class="section-title">ADC Readings (mV)</div>';
    html += '<table class="data-table"><tr><th>Signal</th><th>Chip 0</th><th>Chip 1</th></tr>';
    for (let ch = 0; ch < 4; ch++) {
        html += '<tr><td>' + CHANNEL_LABELS[0][ch] + '</td>';
        html += '<td id="adc-' + feb.id + '-0-' + ch + '">' + Math.round(feb.adc[0][ch]) + '</td>';
        html += '<td id="adc-' + feb.id + '-1-' + ch + '">' + Math.round(feb.adc[1][ch]) + '</td></tr>';
    }
    html += '</table>';

    html += '<div class="section-title">DAC Controls (mV)</div>';
    html += '<table class="data-table"><tr><th>Signal</th><th>Chip 0</th><th>Chip 1</th></tr>';
    for (let ch = 0; ch < 4; ch++) {
        html += '<tr><td>' + CHANNEL_LABELS[0][ch] + '</td>';
        for (let c = 0; c < 2; c++) {
            const target = Math.round(feb.dac.target[c][ch]);
            const actual = Math.round(feb.adc[c][ch]);
            const enabled = feb.dac.enabled[c];
            html += '<td>';
            html += '<input type="number" id="dac-' + feb.id + '-' + c + '-' + ch + '" value="' + target + '" min="0" max="5000" step="10">';
            html += ' <button id="setbtn-' + feb.id + '-' + c + '-' + ch + '" onclick="setDAC(' + feb.id + ',' + c + ',' + (ch + 1) + ')" ' + (!enabled ? 'disabled' : '') + '>Set</button>';
            html += '<br><span class="actual-voltage" id="actual-' + feb.id + '-' + c + '-' + ch + '">ADC: ' + actual + ' mV</span>';
            html += '</td>';
        }
        html += '</tr>';
    }
    html += '</table>';

    html += '<div class="section-title">DAC Power</div>';
    html += '<div class="btn-group">';
    for (let c = 0; c < 2; c++) {
        const en = feb.dac.enabled[c];
        html += '<span>Chip ' + c + ': ';
        html += '<button class="enable" id="enable-' + feb.id + '-' + c + '" onclick="enableDAC(' + feb.id + ',' + c + ')" ' + (en ? 'disabled' : '') + '>Enable</button> ';
        html += '<button class="disable" id="disable-' + feb.id + '-' + c + '" onclick="disableDAC(' + feb.id + ',' + c + ')" ' + (!en ? 'disabled' : '') + '>Disable</button>';
        html += '</span> ';
    }
    html += '</div>';

    card.innerHTML = html;
    return card;
}

function refreshFebCard(feb, card) {
    byId('pcf-' + feb.id, e => e.textContent = 'PCF: ' + feb.pcf.toString(2).padStart(8, '0'));

    for (let c = 0; c < 2; c++) {
        byId('temp-' + feb.id + '-' + c, e => e.textContent = feb.temp[c].toFixed(1));
    }

    for (let c = 0; c < 2; c++) {
        for (let ch = 0; ch < 4; ch++) {
            byId('adc-' + feb.id + '-' + c + '-' + ch, e => e.textContent = Math.round(feb.adc[c][ch]));
        }
    }

    for (let c = 0; c < 2; c++) {
        const en = feb.dac.enabled[c];
        byId('enable-' + feb.id + '-' + c, e => e.disabled = en);
        byId('disable-' + feb.id + '-' + c, e => e.disabled = !en);

        for (let ch = 0; ch < 4; ch++) {
            byId('actual-' + feb.id + '-' + c + '-' + ch, e => e.textContent = 'ADC: ' + Math.round(feb.adc[c][ch]) + ' mV');
            byId('setbtn-' + feb.id + '-' + c + '-' + ch, e => e.disabled = !en);
        }
    }
}

function byId(id, fn) {
    const el = document.getElementById(id);
    if (el) fn(el);
}

function updateFebPage(data) {
    const params = new URLSearchParams(window.location.search);
    const febId = parseInt(params.get('id')) || 0;
    const container = document.getElementById('feb-detail');
    if (!container) return;

    const feb = data.febs[febId];
    if (!feb) {
        container.innerHTML = '<p class="error-msg">FEB ' + febId + ' not found</p>';
        return;
    }

    let card = document.getElementById('feb-card-' + febId);
    if (!card) {
        container.innerHTML = '';
        card = createFebCard(feb);
        container.appendChild(card);
    } else {
        refreshFebCard(feb, card);
    }
}

function setDAC(feb, chip, channel) {
    const input = document.getElementById('dac-' + feb + '-' + chip + '-' + (channel - 1));
    if (!input) return;
    const voltage = parseFloat(input.value);
    if (isNaN(voltage)) return;

    const btn = document.getElementById('setbtn-' + feb + '-' + chip + '-' + (channel - 1));
    if (btn) { btn.textContent = '...'; btn.disabled = true; }

    const params = 'feb=' + encodeURIComponent(feb) +
        '&chip=' + encodeURIComponent(chip) +
        '&channel=' + encodeURIComponent(channel) +
        '&voltage=' + encodeURIComponent(voltage);
    fetch('/api/dac', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: params
    })
    .then(r => r.json())
    .then(() => { if (btn) { btn.textContent = 'Set'; btn.disabled = false; } fetchStatus(); })
    .catch(() => { if (btn) { btn.textContent = 'Set'; btn.disabled = false; } });
}

function enableDAC(feb, chip) {
    const params = 'feb=' + encodeURIComponent(feb) + '&chip=' + encodeURIComponent(chip);
    fetch('/api/dac/enable', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: params
    })
    .then(r => r.json())
    .then(() => fetchStatus())
    .catch(err => console.error(err));
}

function disableDAC(feb, chip) {
    const params = 'feb=' + encodeURIComponent(feb) + '&chip=' + encodeURIComponent(chip);
    fetch('/api/dac/disable', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: params
    })
    .then(r => r.json())
    .then(() => fetchStatus())
    .catch(err => console.error(err));
}

function enableAllDACs() {
    fetch('/api/dac/enableall', {method:'POST'})
    .then(r => r.json())
    .then(() => fetchStatus())
    .catch(err => console.error(err));
}

function disableAllDACs() {
    fetch('/api/dac/disableall', {method:'POST'})
    .then(r => r.json())
    .then(() => fetchStatus())
    .catch(err => console.error(err));
}

function setAllFEBs(type) {
    const input = document.getElementById(type + '-voltage');
    if (!input) return;
    const voltage = parseFloat(input.value);
    if (isNaN(voltage)) return;

    const params = 'voltage=' + encodeURIComponent(voltage) + '&type=' + encodeURIComponent(type);
    fetch('/api/dac/setall', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: params
    })
    .then(r => r.json())
    .then(() => fetchStatus())
    .catch(err => console.error(err));
}

function setFEB(feb, type) {
    const input = document.getElementById(type + '-voltage');
    if (!input) return;
    const voltage = parseFloat(input.value);
    if (isNaN(voltage)) return;

    const params = 'feb=' + encodeURIComponent(feb) +
        '&voltage=' + encodeURIComponent(voltage) +
        '&type=' + encodeURIComponent(type);
    fetch('/api/dac/setfeb', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: params
    })
    .then(r => r.json())
    .then(() => fetchStatus())
    .catch(err => console.error(err));
}

function getCurrentFEB() {
    const params = new URLSearchParams(window.location.search);
    return parseInt(params.get('id')) || 0;
}

function setCurrentFEB(type) {
    setFEB(getCurrentFEB(), type);
}

function exportConfig() {
    fetch('/api/config/export')
        .then(r => r.blob())
        .then(blob => {
            const a = document.createElement('a');
            a.href = URL.createObjectURL(blob);
            a.download = 'dac_config.json';
            a.click();
            URL.revokeObjectURL(a.href);
        })
        .catch(err => console.error(err));
}

function importConfig() {
    const input = document.getElementById('import-file');
    if (!input || !input.files[0]) return;
    const reader = new FileReader();
    reader.onload = function(e) {
        const data = encodeURIComponent(e.target.result);
        fetch('/api/config/import', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: 'data=' + data
        })
        .then(r => r.json())
        .then(() => fetchStatus())
        .catch(err => console.error(err));
    };
    reader.readAsText(input.files[0]);
}
