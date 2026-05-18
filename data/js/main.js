let chart = null;
let sensorData = null;

const datasets = {
    temperatuur:    { label: "Luchttemperatuur (°C)", key: "temperatureHistory", color: "#ff6384" },
    luchtvochtigheid: { label: "Luchtvochtigheid (%)",   key: "humidityHistory",    color: "#36a2eb" },
    luchtdruk:      { label: "Luchtdruk (hPa)",       key: "pressureHistory",    color: "#4bc0c0" },
};

let activeDataset = "temperatuur";

async function fetchData() {
    const response = await fetch("/data");
    sensorData = await response.json();

    // update cards
    document.getElementById("temperatuur").textContent    = sensorData.temperatuur + "°C";
    document.getElementById("luchtvochtigheid").textContent = sensorData.luchtvochtigheid + "%";
    document.getElementById("luchtdruk").textContent      = sensorData.luchtdruk + " hPa";

    updateChart();
}

function updateChart() {
    if (!sensorData) return;

    const ds = datasets[activeDataset];
    const history = sensorData[ds.key];
    const labels = sensorData.timestamps.map(h => "T+" + h + "u");

    if (chart) {
        chart.data.labels = labels;
        chart.data.datasets[0].data = history;
        chart.data.datasets[0].label = ds.label;
        chart.data.datasets[0].borderColor = ds.color;
        chart.update();
    } else {
        const ctx = document.getElementById("sensorChart").getContext("2d");
        chart = new Chart(ctx, {
            type: "line",
            data: {
                labels: labels,
                datasets: [{
                    label: ds.label,
                    data: history,
                    borderColor: ds.color,
                    backgroundColor: ds.color + "33",
                    tension: 0.3,
                    fill: true,
                }]
            },
            options: {
                responsive: true,
                plugins: { legend: { display: false } },
                scales: { y: { beginAtZero: false } }
            }
        });
    }
}

// clicking cards changes graph
document.querySelectorAll(".sensor-card").forEach(card => {
    card.style.cursor = "pointer";
    card.addEventListener("click", () => {
        activeDataset = card.dataset.sensor;
        updateChart();
    });
});

// refresh
fetchData();
setInterval(fetchData, 5000);