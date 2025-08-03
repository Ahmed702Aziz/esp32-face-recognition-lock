import cv2
import os
import pickle
from datetime import datetime
from flask import Flask, render_template_string, Response, jsonify, request
import face_recognition
import threading
import paho.mqtt.client as mqtt
import time
app = Flask(__name__)

# Set up the camera URL 
camera_url = 'http://192.168.1.13/stream'

# Directory for VIP face images
vip_faces_dir = os.path.join(os.getcwd(), 'VIP_faces')
os.makedirs(vip_faces_dir, exist_ok=True)

# File path for face encodings
vip_encodings_path = os.path.join(vip_faces_dir, 'vip_encodings.pkl')

# Load known VIP face encodings if available
if os.path.exists(vip_encodings_path):
    with open(vip_encodings_path, 'rb') as f:
        vip_encodings = pickle.load(f)
else:
    vip_encodings = {}

# MQTT setup
mqtt_broker_url = '94fbaebfe12e4662be30e6222d1af83d.s1.eu.hivemq.cloud'
mqtt_port = 8883
mqtt_username = 'ahmedaziz'
mqtt_password = 'Aziz1234567'
mqtt_topic = 'door/control'

# Create MQTT client
mqtt_client = mqtt.Client()
mqtt_client.username_pw_set(mqtt_username, mqtt_password)
mqtt_client.tls_set()  #  SSL for secure communication

# Connect to the MQTT broker
def connect_mqtt():
    try:
        mqtt_client.connect(mqtt_broker_url, mqtt_port, keepalive=60)
        mqtt_client.loop_start()
        time.sleep(1)  # Allow time for connection
    except Exception as e:
        print(f"MQTT Connection failed: {e}")
# Publish message to MQTT broker
def send_mqtt_command(command):
    try:
        msg_info = mqtt_client.publish(
            mqtt_topic, 
            payload=command, 
            qos=1,  # Qos level 1
            retain=False
        )
        msg_info.wait_for_publish()  # Wait for confirmation
        print(f"DEBUG - Published to {mqtt_topic}: '{command}' (mid={msg_info.mid})")
    except Exception as e:
        print(f"Error publishing MQTT message: {e}")# Open ESP32-CAM stream
cap = cv2.VideoCapture(camera_url)
if not cap.isOpened():
    print("Error: Could not open camera stream.")
else:
    print("Camera stream opened successfully.")

detected_faces_count = 0
last_frame = None  # To store the most recent frame
frame_counter = 0  # To limit face detection frequency

# Lock for thread synchronization
frame_lock = threading.Lock()

# Face recognition and drawing
def detect_faces(frame):
    global detected_faces_count, frame_counter
    if frame_counter % 1 == 0:  # Process every frame 
        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        face_locations = face_recognition.face_locations(rgb_frame)
        face_encodings = face_recognition.face_encodings(rgb_frame, face_locations)
        detected_faces_count = len(face_locations)

        for (top, right, bottom, left), encoding in zip(face_locations, face_encodings):
            name = "Unknown"
            if vip_encodings:
                matches = face_recognition.compare_faces(list(vip_encodings.values()), encoding)
                face_distances = face_recognition.face_distance(list(vip_encodings.values()), encoding)
                best_match_index = face_distances.argmin() if face_distances.size > 0 else None

                if best_match_index is not None and matches[best_match_index]:
                    name = list(vip_encodings.keys())[best_match_index]

            # Send MQTT command when VIP face is detected
            if name != "Unknown":
                send_mqtt_command("door/open")  # Open the door for VIPs
            else:
                send_mqtt_command("door/close")  # Close the door for unknown faces

            # Draw rectangle and name on the frame
            cv2.rectangle(frame, (left, top), (right, bottom), (0, 255, 0), 2)
            cv2.putText(frame, name, (left, top - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)
    
    frame_counter += 1
    return frame

# Stream frames from the camera
def capture_frames():
    global last_frame
    while True:
        ret, frame = cap.read()
        if not ret:
            continue

        with frame_lock:
            # Resize frame to a smaller resolution for faster processing 
            frame_resized = cv2.resize(frame, (320, 240))
            last_frame = frame_resized

def gen_frames():
    global last_frame
    while True:
        if last_frame is not None:
            frame = last_frame.copy()
            frame = detect_faces(frame)  # Ensure faces are drawn on every frame

            # Encode the frame as JPEG
            ret, buffer = cv2.imencode('.jpg', frame)
            if not ret:
                continue
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + buffer.tobytes() + b'\r\n')

@app.route('/')
def index():
    return render_template_string('''
    <!DOCTYPE html>
    <html>
    <head>
        <title>VIP Face Recognition</title>
        <style>
            body { font-family: Arial; background-color: #f0f0f0; text-align: center; }
            .container { margin: 20px auto; padding: 20px; background: white; width: 70%; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }
            img { border-radius: 10px; border: 3px solid #4CAF50; }
            input, button { padding: 10px; font-size: 16px; margin: 10px; }
            #message { color: green; font-weight: bold; }
        </style>
    </head>
    <body>
        <div class="container">
            <h1>Real-Time VIP Face Recognition</h1>
            <img src="{{ url_for('video_feed') }}" width="640" height="480" />
            <p id="face_count">Faces Detected: 0</p>
            <input type="text" id="vip_name" placeholder="Enter VIP Name">
            <button onclick="saveVipFace()">Save VIP Face</button>
            <p id="message"></p>
        </div>

        <script>
            function fetchFaceCount() {
                fetch('/update_face_count')
                    .then(res => res.json())
                    .then(data => {
                        document.getElementById("face_count").innerText = "Faces Detected: " + data.face_count;
                    });
            }

            setInterval(fetchFaceCount, 1000);

            function saveVipFace() {
                const name = document.getElementById("vip_name").value;
                if (!name) {
                    alert("Please enter a VIP name!");
                    return;
                }
                fetch('/save_vip_face', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ name: name })
                })
                .then(res => res.json())
                .then(data => {
                    document.getElementById("message").innerText = data.message;
                })
                .catch(err => {
                    console.error(err);
                    alert("Error saving face.");
                });
            }
        </script>
    </body>
    </html>
    ''')

@app.route('/video_feed')
def video_feed():
    return Response(gen_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/update_face_count')
def update_face_count():
    return jsonify({'face_count': detected_faces_count})

@app.route('/save_vip_face', methods=['POST'])
def save_vip_face():
    global last_frame
    data = request.get_json()
    vip_name = data.get('name', '').strip()

    if not vip_name:
        return jsonify({'message': 'Name is required'}), 400

    if last_frame is None:
        return jsonify({'message': 'No frame available'}), 500

    rgb_frame = cv2.cvtColor(last_frame, cv2.COLOR_BGR2RGB)
    face_locations = face_recognition.face_locations(rgb_frame)
    face_encodings = face_recognition.face_encodings(rgb_frame, face_locations)

    if len(face_encodings) == 0:
        return jsonify({'message': 'No face detected in current frame'}), 400

    # Save image
    person_dir = os.path.join(vip_faces_dir, vip_name)
    os.makedirs(person_dir, exist_ok=True)
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    image_path = os.path.join(person_dir, f"{vip_name}_{timestamp}.jpg")
    cv2.imwrite(image_path, last_frame)

    # Save encoding
    vip_encodings[vip_name] = face_encodings[0]
    with open(vip_encodings_path, 'wb') as f:
        pickle.dump(vip_encodings, f)

    print(f" Saved VIP face and encoding for: {vip_name}")
    return jsonify({'message': f"VIP face of {vip_name} saved."})

if __name__ == '__main__':
    # Connect to MQTT broker
    connect_mqtt()

    # Start the frame capture in a separate thread
    capture_thread = threading.Thread(target=capture_frames)
    capture_thread.daemon = True
    capture_thread.start()

    app.run(host='0.0.0.0', port=5000)
