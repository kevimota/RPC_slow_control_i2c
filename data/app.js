let statusInterval = null;

document.addEventListener('DOMContentLoaded', function() {
    const page = document.body.dataset.page;
    if (page === 'dashboard') {
        fetchStatus();
        statusInterval = setInterval(fetchStatus, 2000);
    } else if (page === 'feb') {
        fetchStatus();
        statusInterval = setInterval(fetchStatus, 2000);
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
    container.innerHTML = '';

    data.febs.forEach(feb => {
        const card = createFebCard(feb);
        container.appendChild(card);
    });
}

function createFebCard(feb) {
    const card = document.createElement('div');
    card.className = 'feb-card';
    card.id = 'feb-' + feb.id;

    const pcfStr = feb.pcf.toString(2).padStart(8, '0');

    let html = '<h2>FEB ' + feb.id + ' <span class="pcf-badge">PCF: ' + pcfStr + '</span></h2>';

    html += '<div class="section-title">Temperature</div>';
    html += '<div class="temp-display">Chip 0: ' + feb.temp[0].toFixed(1) + ' &deg;C &nbsp;|&nbsp; Chip 1: ' + feb.temp[1].toFixed(1) + ' &deg;C</div>';

    html += '<div class="section-title">ADC Readings (mV)</div>';
    html += '<table><tr><th>Ch</th><th>Chip 0</th><th>Chip 1</th></tr>';
    for (let ch = 0; ch < 4; ch++) {
        html += '<tr><td>' + (ch + 1) + '</td>';
        html += '<td>' + Math.round(feb.adc[0][ch]) + '</td>';
        html += '<td>' + Math.round(feb.adc[1][ch]) + '</td></tr>';
    }
    html += '</table>';

    html += '<div class="section-title">DAC Controls (mV)</div>';
    html += '<table><tr><th>Ch</th><th>Chip 0</th><th>Chip 1</th></tr>';
    for (let ch = 0; ch < 4; ch++) {
        html += '<tr><td>' + (ch + 1) + '</td>';
        for (let c = 0; c < 2; c++) {
            const target = Math.round(feb.dac.target[c][ch]);
            const actual = Math.round(feb.adc[c][ch]);
            const enabled = feb.dac.enabled[c];
            html += '<td>';
            html += '<input type="number" id="dac-' + feb.id + '-' + c + '-' + ch + '" value="' + target + '" min="0" max="5000" step="10">';
            html += ' <button onclick="setDAC(\'' + feb.id + '\',' + c + ',' + (ch + 1) + ')" ' + (!enabled ? 'disabled' : '') + '>Set</button>';
            html += '<br><span class="actual-voltage">ADC: ' + actual + ' mV</span>';
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
        html += '<button class="enable" onclick="enableDAC(\'' + feb.id + '\',' + c + ')" ' + (en ? 'disabled' : '') + '>Enable</button>';
        html += ' ';
        html += '<button class="disable" onclick="disableDAC(\'' + feb.id + '\',' + c + ')" ' + (!en ? 'disabled' : '') + '>Disable</button>';
        html += '</span> ';
    }
    html += '</div>';

    card.innerHTML = html;
    return card;
}

function updateFebPage(data) {
    const params = new URLSearchParams(window.location.search);
    const febId = params.get('id') || 'A';
    const container = document.getElementById('feb-detail');
    if (!container) return;

    const feb = data.febs.find(f => f.id === febId);
    if (!feb) {
        container.innerHTML = '<p class="error-msg">FEB ' + febId + ' not found</p>';
        return;
    }

    container.innerHTML = '';
    const card = createFebCard(feb);
    container.appendChild(card);
}

function setDAC(feb, chip, channel) {
    const input = document.getElementById('dac-' + feb + '-' + chip + '-' + (channel - 1));
    if (!input) return;
    const voltage = parseFloat(input.value);
    if (isNaN(voltage)) return;

    const btn = input.nextElementSibling;
    btn.textContent = '...';
    btn.disabled = true;

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
    .then(data => {
        btn.textContent = 'Set';
        btn.disabled = false;
        fetchStatus();
    })
    .catch(err => {
        btn.textContent = 'Set';
        btn.disabled = false;
        console.error('DAC error:', err);
    });
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
    const dparams = 'feb=' + encodeURIComponent(feb) + '&chip=' + encodeURIComponent(chip);
    fetch('/api/dac/disable', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: dparams
    })
    .then(r => r.json())
    .then(() => fetchStatus())
    .catch(err => console.error(err));
}
