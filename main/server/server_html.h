#pragma once

static const char HTML_TEMPLATE[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Water Meter ESP32-S3</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; margin: 20px; background: #f0f8ff; }
        .box { background: white; max-width: 700px; margin: 20px auto; padding: 30px; border-radius: 12px; box-shadow: 0 4px 20px rgba(0,0,0,0.1); }
        #digits { font-size: 4em; font-weight: bold; color: #0066cc; margin: 30px 0; letter-spacing: 10px; }
        #roi-img { max-width: 100%; height: auto; border: 3px solid #0066cc; border-radius: 8px; margin: 20px 0; }
        #download-btn { padding: 12px 24px; font-size: 18px; background: #0066cc; color: white; border: none; border-radius: 8px; cursor: pointer; box-shadow: 0 4px 8px rgba(0,0,0,0.2); margin: 20px 0; }
        #download-btn:hover { background: #0055aa; }
        .status { color: #666; font-style: italic; }
    </style>
</head>
<body>
    <div class="box">
        <h1>Water Meter Monitor</h1>
        
        <div id="digits">-----</div>
        
        <p><strong>Captured ROI:</strong></p>
        <img id="roi-img" src="/roi.jpg?t=" alt="ROI from camera">

        <br><br>
        <button id="download-btn" onclick="downloadPhoto()">
            Download current photo
        </button>

        <p class="status" id="status">Loading...</p>
        <hr>
        <small>ESP32-S3 &bull; Live view & readings</small>
    </div>

    <script>
        const UPDATE_INTERVAL_MS = %d;

        function updateReadings() {
            fetch('/readings')
                .then(r => { if (!r.ok) throw Error(r.status); return r.json(); })
                .then(data => {
                    document.getElementById('digits').textContent = data.digits;
                    document.getElementById('status').textContent = 'Updated: ' + new Date().toLocaleTimeString();
                })
                .catch(err => {
                    document.getElementById('digits').textContent = 'ERROR';
                    document.getElementById('status').textContent = 'Readings error: ' + err;
                });
        }

        function downloadPhoto() {
            const link = document.createElement('a');
            link.href = '/download.jpg?t=' + new Date().getTime();
            link.download = '';
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
        }

        function updateImage() {
            const img = document.getElementById('roi-img');
            img.src = '/roi.jpg?t=' + new Date().getTime();
        }

        updateReadings();
        updateImage();
        setInterval(updateReadings, UPDATE_INTERVAL_MS);
        setInterval(updateImage, UPDATE_INTERVAL_MS);
    </script>
</body>
</html>
)rawliteral";