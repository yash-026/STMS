// Connect to Socket.IO server
const socket = io();

// Update UI when receiving traffic data
socket.on('trafficUpdate', function(data) {
    // Update traffic status
    const trafficStatus = document.getElementById('traffic-status');
    if (trafficStatus) {
        trafficStatus.textContent = data.trafficLevel;
        trafficStatus.className = ''; // Reset class
        if (data.trafficLevel.toLowerCase() === 'low') {
            trafficStatus.classList.add('low');
        } else if (data.trafficLevel.toLowerCase() === 'medium') {
            trafficStatus.classList.add('medium');
        } else if (data.trafficLevel.toLowerCase() === 'high') {
            trafficStatus.classList.add('high');
        }
    }

    // Update vehicle count
    const vehicleCount = document.getElementById('vehicle-count');
    if (vehicleCount) {
        vehicleCount.textContent = data.vehicleCount;
    }

    // Update last updated time
    const lastUpdated = document.getElementById('last-updated');
    if (lastUpdated) {
        lastUpdated.textContent = data.lastUpdated;
    }

    // Update priority vehicle status and timestamp
    const priorityStatus = document.getElementById('priority-status');
    const priorityTimestamp = document.getElementById('priority-timestamp');
    if (priorityStatus && priorityTimestamp) {
        if (data.priorityVehicles) {
            priorityStatus.textContent = 'Detected';
            priorityStatus.classList.add('detected');
            priorityTimestamp.textContent += `Timestamp: ${data.priorityTimestamp}`;
        } else {
            priorityStatus.textContent = 'Not Detected';
            priorityStatus.classList.remove('detected');
            priorityTimestamp.textContent = '';
        }
    }

    // Handle fire alert
    const alertCard = document.getElementById('alert-card');
    const alertBox = document.getElementById('alert-box');
    if (alertCard && alertBox) {
        if (data.fireAlert) {
            alertCard.style.display = 'none';
            alertBox.style.display = 'none';
            alertBox.textContent = "Fire Detected! Smoke level: "
            alertCard.classList.add('fire');
            const smokeLevel = document.getElementById('smoke-level');
            if (smokeLevel) {
                smokeLevel.textContent = data.smokeLevel;
            }
        } else {
            alertCard.style.display = 'block';
            alertBox.style.display = 'block';
            alertBox.textContent = 'Fire Not Detected!';
            alertCard.classList.remove('fire');
        }
    }
});

// Handle historical data
socket.on('historicalData', function(data) {
    const historyDiv = document.getElementById('historical-data');
    if (!historyDiv) return;

    if (data.length === 0) {
        historyDiv.innerHTML = 'No historical data available for the selected period';
        return;
    }

    let html = '<table style="width:100%; border-collapse: collapse;">';
    html += '<tr><th style="text-align:left; padding:8px; border-bottom:1px solid #ddd;">Time</th>';
    html += '<th style="text-align:left; padding:8px; border-bottom:1px solid #ddd;">Traffic Level</th>';
    html += '<th style="text-align:left; padding:8px; border-bottom:1px solid #ddd;">Count</th></tr>';

    data.forEach(entry => {
        const date = new Date(entry.timestamp);
        let timeStr;

        // Format time based on the selected period
        if (entry.period === 'second') {
            timeStr = date.toLocaleTimeString([], { minute: '2-digit', second: '2-digit' });
        } else if (entry.period === 'minute') {
            timeStr = date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
        } else {
            timeStr = date.toLocaleTimeString([], { hour: '2-digit' }) + ':00';
        }

        html += `<tr>
            <td style="padding:8px; border-bottom:1px solid #ddd;">${timeStr}</td>
            <td style="padding:8px; border-bottom:1px solid #ddd;">${entry.traffic_level}</td>
            <td style="padding:8px; border-bottom:1px solid #ddd;">${entry.count}</td>
        </tr>`;
    });

    html += '</table>';
    historyDiv.innerHTML = html;
});

// Function to request historical data
function getHistoricalData(period) {
    socket.emit('getHistoricalData', period);
}

// Handle socket errors
socket.on('error', function(errorMsg) {
    console.error('Socket error:', errorMsg);
    alert('Error: ' + errorMsg);
});