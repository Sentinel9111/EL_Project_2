let chart = null;
let sensorData = null;

// chart settings
const datasets = {
    temperatuur: { label: "Luchttemperatuur (°C)", key: "temperatureHistory", color: "#ff6384", precision: 1, stepSize: 0.1 },
    luchtvochtigheid: { label: "Luchtvochtigheid (%)", key: "humidityHistory", color: "#36a2eb", precision: 0, beginAtZero: true, suggestedMax: 100 },
    luchtdruk: { label: "Luchtdruk (hPa)", key: "pressureHistory", color: "#4bc0c0", precision: 0},
    watertemperatuur: { label: "Watertemperatuur (°C)", key: "waterTemperatureHistory", color: "#0077b6", precision: 1, stepSize: 0.1 },
    troebelheid: { label: "Troebelheid (NTU)", key: "turbidityHistory", color: "#e9c46a", precision: 0 },
};

let activeDataset = "temperatuur";

// fetch data from ESP32
async function fetchData() {
    const response = await fetch("/api/data");
    sensorData = await response.json();

    // update cards
    document.getElementById("temperatuur").textContent = sensorData.temperatuur + "°C";
    document.getElementById("luchtvochtigheid").textContent = sensorData.luchtvochtigheid + "%";
    document.getElementById("luchtdruk").textContent = sensorData.luchtdruk + " hPa";
    document.getElementById("watertemperatuur").textContent = sensorData.watertemperatuur + "°C";
    document.getElementById("troebelheid").textContent = sensorData.troebelheid + " NTU";

    updateAdvies(
        "zwem",
        sensorData.zwemrisico,
        sensorData.risicostijging
    );
    updateAdvies(
        "vis",
        sensorData.visrisico,
        sensorData.risicostijging
    );
    updateAdvies(
        "algen",
        sensorData.algengroei,
        sensorData.risicostijging
    );

    updateChart();
}

// update chart.js chart
function updateChart() {
    if (!sensorData) return;
    // retrieve chart title
    const ds = datasets[activeDataset];
    document.getElementById("chartTitle").textContent = ds.label;
    // retrieve history arrays
    let history = sensorData[ds.key];
    // retrieve date and time
    const labels = sensorData.timestamps.map(t => {
        const d = new Date(t * 1000);
        return d.toLocaleDateString("nl-NL", { day: "2-digit", month: "long", hour: "2-digit", minute: "2-digit" });
    })
    // round numbers
    history = history.map(value => {
        return parseFloat(Number(value).toFixed(ds.precision))
    })
    // import chart settings
    if (chart) {
        chart.data.labels = labels;
        chart.data.datasets[0].data = history;
        chart.data.datasets[0].label = ds.label;
        chart.data.datasets[0].borderColor = ds.color;
        chart.options.scales.y.beginAtZero = ds.beginAtZero;
        chart.options.scales.y.suggestedMax = ds.suggestedMax;
        chart.options.scales.y.ticks.precision = ds.precision;
        chart.options.scales.y.ticks.stepSize = ds.stepSize;
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
                scales: {
                    y: {
                        beginAtZero: ds.beginAtZero,
                        suggestedMax: ds.suggestedMax,
                        ticks: {
                            precision: ds.precision,
                            stepSize: ds.stepSize,
                            callback: function(value) {
                                return parseFloat(value);
                            }
                        }
                    }
                }
            }
        });
    }
}

// update advice cards
function updateAdvies(type, risico, voorspelling) {
    const card   = document.getElementById(type + "adviesCard");
    const badge  = document.getElementById(type + "adviesBadge");
    const risicoEl = document.getElementById(type + "risico");
    const footer = document.getElementById(type + "adviesVoorspelling");

    risicoEl.textContent = risico;

    // badge text and colour based on risk
    if (risico <= 15) { // 1 1 1
        badge.textContent = type === "zwem" ? "Uitstekend om in te zwemmen." : type === "vis" ? "Uitstekend voor vissen." : "Zeer lage kans op algengroei.";
        badge.className = "badge p-3 fs-4 bg-success text-dark";
        card.className = "card h-100 shadow-sm border-success";
    } else if (risico <= 20) { // 2 1 1
        badge.textContent = type === "zwem" ? "Goed om in te zwemmen." : type === "vis" ? "Uitstekend voor vissen." : "Zeer lage kans op algengroei.";
        badge.className = "badge p-3 fs-4 bg-warning text-dark";
        card.className = "card h-100 shadow-sm border-success";
    } else if (risico <= 35) { // 2 2 2
        badge.textContent = type === "zwem" ? "Goed om in te zwemmen." : type === "vis" ? "Goed voor vissen." : "Lage kans op algengroei.";
        badge.className = "badge p-3 fs-4 bg-warning text-dark";
        card.className = "card h-100 shadow-sm border-success";
    } else if (risico <= 40) { // 3 2 2
        badge.textContent = type === "zwem" ? "Redelijk, houd de waterkwaliteit in de gaten." : type === "vis" ? "Goed voor vissen." : "Lage kans op algengroei.";
        badge.className = "badge p-3 fs-4 bg-warning text-dark";
        card.className = "card h-100 shadow-sm border-warning";
    } else if (risico <= 60) { // 3 3 3
        badge.textContent = type === "zwem" ? "Redelijk, houd de waterkwaliteit in de gaten." : type === "vis" ? "Kan lichte stress veroorzaken voor vissen." : "Algengroei is mogelijk.";
        badge.className = "badge p-3 fs-4 bg-warning text-dark";
        card.className = "card h-100 shadow-sm border-warning";
    } else if (risico <= 80) { // 4 4 4
        badge.textContent = type === "zwem" ? "Zwemmen wordt afgeraden." : type === "vis" ? "Ongezond voor vissen." : "Verhoogde kans op algengroei.";
        badge.className = "badge p-3 fs-4 bg-warning text-dark";
        card.className = "card h-100 shadow-sm border-danger";
    } else { // 5 5 5
        badge.textContent = type === "zwem" ? "Ongeschikt om in te zwemmen." : type === "vis" ? "Gevaarlijk voor vissen." : "Grote kans op algengroei.";
        badge.className = "badge p-3 fs-4 bg-danger text-dark";
        card.className = "card h-100 shadow-sm border-danger";
    }

    // risk prediction
    if (voorspelling === 1) {
        footer.textContent = "↑ Risico stijgt";
        footer.className = "text-danger";
    } else if (voorspelling === 2) {
        footer.textContent = "↓ Risico daalt";
        footer.className = "text-success";
    } else {
        footer.textContent = "→ Risico stabiel";
        footer.className = "text-muted";
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
setInterval(fetchData, 5000); // 3600000 for 1 hour