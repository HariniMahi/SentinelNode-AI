#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <vector>

// ================= HARDWARE =================
const int POWER_LED = 2;     
const int ALERT_LED = 4;     
const int BUZZER_PIN = 15;   

const int buzzerChannel = 0;
const int buzzerFreq = 1000;
const int buzzerResolution = 8;

// ================= WIFI =================
const char* ssid = "cmscet tp2";
const char* password = "Welcometocmscet";

// ================= BACKEND =================
const char* serverUrl = "http://192.168.0.217:8000/api/log";

WebServer server(80);

struct Attempt {
  String ip;
  String user;
  String pass;
};

std::vector<Attempt> attempts;

// Backend buffer
String pendingJSON = "";
bool sendToBackend = false;

// ================= HTML =================
String getHTMLPage() {
  return R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
      <title>Secure Login</title>
      <style>
          body { font-family: Arial; background:#0f172a; color:white; text-align:center; }
          .box { background:#1e293b; padding:30px; margin-top:100px; width:300px; margin:auto; border-radius:10px; }
          input { width:90%; padding:10px; margin:10px; border-radius:5px; border:none; }
          button { padding:10px 20px; background:#3b82f6; color:white; border:none; border-radius:5px; cursor:pointer; }
      </style>
  </head>
  <body>
      <div class="box">
          <h2>Secure Admin Login</h2>
          <form action="/login" method="POST">
              <input type="text" name="username" placeholder="Username" required><br>
              <input type="password" name="password" placeholder="Password" required><br>
              <button type="submit">Login</button>
          </form>
      </div>
  </body>
  </html>
  )rawliteral";
}

// ================= ROUTES =================
void handleRoot() {
  server.send(200, "text/html", getHTMLPage());
}

void indicateAttack() {
  Serial.println("⚠ Attack detected!");

  digitalWrite(ALERT_LED, HIGH);
  ledcWriteTone(buzzerChannel, buzzerFreq);
  delay(300);
  ledcWriteTone(buzzerChannel, 0);
  digitalWrite(ALERT_LED, LOW);
}

void handleDashboard() {
  String html = "<html><body><h2>Login Attempts</h2><ul>";
  for (auto &a : attempts) {
    html += "<li>" + a.ip + " - " + a.user + " / " + a.pass + "</li>";
  }
  html += "</ul></body></html>";
  server.send(200, "text/html", html);
}

void handleLogin() {

  String username = server.arg("username");
  String passwordInput = server.arg("password");
  String ip = server.client().remoteIP().toString();

  Serial.println("=================================");
  Serial.println("New Login Attempt:");
  Serial.println("IP: " + ip);
  Serial.println("Username: " + username);
  Serial.println("Password: " + passwordInput);

  attempts.push_back({ip, username, passwordInput});

  indicateAttack();

  // Prepare JSON for backend
  pendingJSON = "{";
  pendingJSON += "\"ip\":\"" + ip + "\",";
  pendingJSON += "\"username\":\"" + username + "\",";
  pendingJSON += "\"password\":\"" + passwordInput + "\"";
  pendingJSON += "}";

  sendToBackend = true;

  server.send(200, "text/html", "<h3>Invalid Credentials</h3>");
}

// ================= SETUP =================
void setup() {

  pinMode(POWER_LED, OUTPUT);
  pinMode(ALERT_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.begin(115200);

  ledcSetup(buzzerChannel, buzzerFreq, buzzerResolution);
  ledcAttachPin(BUZZER_PIN, buzzerChannel);

  Serial.println("SentinelNode-AI Starting...");

  WiFi.begin(ssid, password);
  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  digitalWrite(POWER_LED, HIGH);

  server.on("/", handleRoot);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/dashboard", handleDashboard);
  server.begin();

  Serial.println("Honeypot Ready!");
}

// ================= LOOP =================
void loop() {

  server.handleClient();

  if (sendToBackend && WiFi.status() == WL_CONNECTED) {

    WiFiClient client;
    HTTPClient http;

    http.setTimeout(5000);
    http.setReuse(false);

    if (http.begin(client, serverUrl)) {

      http.addHeader("Content-Type", "application/json");

      int httpResponseCode = http.POST(pendingJSON);

      if (httpResponseCode > 0) {
        Serial.print("Backend Response: ");
        Serial.println(httpResponseCode);
      } else {
        Serial.print("Backend Error: ");
        Serial.println(httpResponseCode);
      }

      http.end();

    } else {
      Serial.println("HTTP Begin Failed");
    }

    sendToBackend = false;
  }
}