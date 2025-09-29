<?php
// PHP CGI 测试脚本 - 即使PHP未安装也会显示错误信息
header("Content-Type: text/html");

echo "<!DOCTYPE html>";
echo "<html>";
echo "<head>";
echo "    <title>PHP CGI Test</title>";
echo "    <style>";
echo "        body { font-family: Arial, sans-serif; margin: 40px; background: linear-gradient(135deg, #ff7b7b 0%, #667eea 100%); color: white; }";
echo "        .container { background: rgba(255,255,255,0.1); padding: 30px; border-radius: 15px; backdrop-filter: blur(10px); }";
echo "        h1 { text-align: center; text-shadow: 2px 2px 4px rgba(0,0,0,0.5); }";
echo "        .info { background: rgba(255,255,255,0.2); padding: 15px; border-radius: 8px; margin: 10px 0; }";
echo "    </style>";
echo "</head>";
echo "<body>";
echo "    <div class='container'>";
echo "        <h1>🐘 PHP CGI Test</h1>";

if (function_exists('phpversion')) {
    echo "        <div class='info'>✅ PHP Version: " . phpversion() . "</div>";
} else {
    echo "        <div class='info'>ℹ️ PHP Function Check</div>";
}

echo "        <div class='info'>📅 Server Time: " . date('Y-m-d H:i:s') . "</div>";

if (isset($_SERVER['REQUEST_METHOD'])) {
    echo "        <div class='info'>🔧 Request Method: " . $_SERVER['REQUEST_METHOD'] . "</div>";
}

if (isset($_SERVER['HTTP_USER_AGENT'])) {
    echo "        <div class='info'>🌐 User Agent: " . htmlspecialchars($_SERVER['HTTP_USER_AGENT']) . "</div>";
}

echo "        <div class='info'>🚀 CGI Environment Variables:</div>";
echo "        <div class='info'>";
echo "            REQUEST_METHOD: " . (isset($_ENV['REQUEST_METHOD']) ? $_ENV['REQUEST_METHOD'] : 'Not Set') . "<br>";
echo "            QUERY_STRING: " . (isset($_ENV['QUERY_STRING']) ? $_ENV['QUERY_STRING'] : 'Empty') . "<br>";
echo "            SERVER_SOFTWARE: " . (isset($_ENV['SERVER_SOFTWARE']) ? $_ENV['SERVER_SOFTWARE'] : 'Not Set') . "<br>";
echo "            REMOTE_ADDR: " . (isset($_ENV['REMOTE_ADDR']) ? $_ENV['REMOTE_ADDR'] : 'Not Set');
echo "        </div>";

echo "        <div class='info'>";
echo "            <h3>🎯 Multi-Language CGI Demo</h3>";
echo "            <p>This demonstrates that webserv supports multiple CGI languages with proper configuration!</p>";
echo "        </div>";

echo "    </div>";
echo "</body>";
echo "</html>";
?>