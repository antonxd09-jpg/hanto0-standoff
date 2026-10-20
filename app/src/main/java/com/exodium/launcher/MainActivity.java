package com.exodium.launcher;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.text.InputType;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;
import java.io.*;
import java.security.MessageDigest;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class MainActivity extends Activity {
    private final ExecutorService executor = Executors.newSingleThreadExecutor();
    private final Handler uiHandler = new Handler(Looper.getMainLooper());
    private TextView logView;
    private Process currentProcess;
    private TextView statusText;
    private EditText keyInput;
    
    private final String SECRET = "HantoO_Super_Secret_Salt_2024";
    private final int bgColor = Color.parseColor("#0F1115");
    private final int cardColor = Color.parseColor("#1A1D24");
    private final int textColorPrimary = Color.parseColor("#FFFFFF");
    private final int textColorSecondary = Color.parseColor("#8E929A");
    private final int accentColor = Color.parseColor("#8C9EFF");
    private final int colorGreen = Color.parseColor("#00E676");
    private final int colorRed = Color.parseColor("#FF1744");

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        try {
            ScrollView mainScroll = new ScrollView(this);
            mainScroll.setBackgroundColor(bgColor);
            
            LinearLayout layout = new LinearLayout(this);
            layout.setOrientation(LinearLayout.VERTICAL);
            layout.setPadding(dp(20), dp(40), dp(20), dp(30));

            String deviceId = Settings.Secure.getString(getContentResolver(), Settings.Secure.ANDROID_ID);

            LinearLayout authCard = createCard();
            TextView authTitle = new TextView(this);
            authTitle.setText("License Key");
            authTitle.setTextColor(textColorPrimary);
            authTitle.setTextSize(16);
            authTitle.setTypeface(Typeface.DEFAULT_BOLD);

            TextView devIdText = new TextView(this);
            devIdText.setText("Device ID: " + deviceId);
            devIdText.setTextColor(textColorSecondary);
            devIdText.setTextSize(11);
            devIdText.setTypeface(Typeface.MONOSPACE);
            devIdText.setPadding(0, dp(5), 0, dp(10));
            
            keyInput = new EditText(this);
            keyInput.setHint("KEY-XXXX-YYYY");
            keyInput.setTextColor(textColorPrimary);
            keyInput.setHintTextColor(textColorSecondary);
            keyInput.setTextSize(14);
            keyInput.setTypeface(Typeface.MONOSPACE);
            keyInput.setInputType(InputType.TYPE_CLASS_TEXT);
            keyInput.setBackground(createRoundedBg(Color.parseColor("#0A0A0C"), dp(8)));
            keyInput.setPadding(dp(10), dp(10), dp(10), dp(10));
            
            SharedPreferences prefs = getSharedPreferences("HantoO", Context.MODE_PRIVATE);
            keyInput.setText(prefs.getString("license_key", ""));

            authCard.addView(authTitle);
            authCard.addView(devIdText);
            authCard.addView(keyInput);
            layout.addView(authCard);

            LinearLayout statusCard = createCard();
            TextView statusTitle = new TextView(this);
            statusTitle.setText("Status");
            statusTitle.setTextColor(textColorPrimary);
            statusTitle.setTextSize(16);
            statusTitle.setTypeface(Typeface.DEFAULT_BOLD);
            
            statusText = new TextView(this);
            statusText.setText("○ Waiting for key");
            statusText.setTextColor(textColorSecondary);
            statusText.setTextSize(14);
            statusText.setTypeface(Typeface.MONOSPACE);

            statusCard.addView(statusTitle);
            statusCard.addView(statusText);
            layout.addView(statusCard);

            Button startBtn = new Button(this);
            startBtn.setText("START");
            startBtn.setTextColor(Color.WHITE);
            startBtn.setTextSize(16);
            startBtn.setTypeface(Typeface.DEFAULT_BOLD);
            startBtn.setBackground(createRoundedBg(colorGreen, dp(12)));
            
            LinearLayout.LayoutParams btnParams = new LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT, dp(55));
            btnParams.setMargins(0, dp(10), 0, dp(15));
            startBtn.setLayoutParams(btnParams);
            startBtn.setOnClickListener(v -> {
                String key = keyInput.getText().toString().trim();
                String result = validateKey(key, deviceId);
                if (result.equals("OK")) {
                    prefs.edit().putString("license_key", key).apply();
                    statusText.setText("○ Starting...");
                    statusText.setTextColor(accentColor);
                    executor.submit(this::startProcess);
                } else {
                    statusText.setText("○ " + result);
                    statusText.setTextColor(colorRed);
                    Toast.makeText(this, result, Toast.LENGTH_LONG).show();
                }
            });
            layout.addView(startBtn);

            Button stopBtn = new Button(this);
            stopBtn.setText("STOP");
            stopBtn.setTextColor(Color.WHITE);
            stopBtn.setTextSize(16);
            stopBtn.setTypeface(Typeface.DEFAULT_BOLD);
            stopBtn.setBackground(createRoundedBg(colorRed, dp(12)));
            
            LinearLayout.LayoutParams stopParams = new LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT, dp(50));
            stopParams.setMargins(0, 0, 0, dp(20));
            stopBtn.setLayoutParams(stopParams);
            stopBtn.setOnClickListener(v -> stopProcess());
            layout.addView(stopBtn);

            TextView logLabel = new TextView(this);
            logLabel.setText("SYSTEM LOGS");
            logLabel.setTextColor(textColorSecondary);
            logLabel.setTextSize(11);
            logLabel.setTypeface(Typeface.MONOSPACE);
            logLabel.setPadding(0, dp(10), 0, dp(5));
            layout.addView(logLabel);

            logView = new TextView(this);
            logView.setTextColor(Color.parseColor("#00FFCC"));
            logView.setTextSize(11);
            logView.setTypeface(Typeface.MONOSPACE);
            logView.setBackground(createRoundedBg(Color.parseColor("#0A0A0C"), dp(12)));
            logView.setPadding(dp(15), dp(15), dp(15), dp(15));
            logView.setText("> Ready.");
            
            layout.addView(logView);

            mainScroll.addView(layout);
            setContentView(mainScroll);

        } catch (Throwable t) {
            Toast.makeText(this, "UI Error: " + t.getMessage(), Toast.LENGTH_LONG).show();
        }
    }

    private String validateKey(String key, String currentDeviceId) {
        if (key == null || key.isEmpty()) return "Пустой ключ";

        // ЖЕСТКИЙ БЛОК СТАРЫХ КЛЮЧЕЙ
        if (key.startsWith("HANT-")) {
            return "Старые ключи больше не работают!";
        }

        if (!key.startsWith("KEY-")) return "Неверный формат";
        try {
            String[] parts = key.split("-");
            if (parts.length != 3) return "Ошибка структуры";

            String part1 = parts[1];
            String part2 = parts[2];

            if (part1.length() != 4) return "Неверная длина ключа";

            String timeStr = part1.substring(0, 2);
            String typeStr = part1.substring(2, 3);

            long days = fromBase62(timeStr);
            long expTime = 1704067200L + (days * 86400L);
            long currentTime = System.currentTimeMillis() / 1000;

            if (currentTime > expTime) return "Срок действия истек";

            String expectedSigRaw = sha256(part1 + "HantoO_Super_Secret_Salt_2024");
            long expectedSigLong = Long.parseLong(expectedSigRaw.substring(0, 8), 16) % (62L * 62L * 62L * 62L);
            String expectedSigStr = toBase62_4(expectedSigLong);

            if (!part2.equals(expectedSigStr)) return "Ключ подделан";

            if (typeStr.equals("S")) {
                SharedPreferences prefs = getSharedPreferences("HantoO", Context.MODE_PRIVATE);
                String savedDevice = prefs.getString("bound_device_id", "");
                if (savedDevice.isEmpty()) {
                    prefs.edit().putString("bound_device_id", currentDeviceId).apply();
                } else if (!savedDevice.equals(currentDeviceId)) {
                    return "Ключ уже активирован на другом устройстве";
                }
            }
            return "OK";
        } catch (Exception e) {
            return "Ошибка парсинга ключа";
        }
    }

    private long fromBase62(String str) {
        String chars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        long num = 0;
        for (char c : str.toCharArray()) {
            num = num * 62 + chars.indexOf(c);
        }
        return num;
    }

    private String toBase62_4(long num) {
        String chars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        if (num == 0) return "0000";
        StringBuilder res = new StringBuilder();
        while (num > 0) {
            res.insert(0, chars.charAt((int) (num % 62)));
            num /= 62;
        }
        return res.toString().length() > 4 ? res.toString().substring(0, 4) : res.toString();
    }

    private String sha256(String base) {
        try {
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            byte[] hash = digest.digest(base.getBytes("UTF-8"));
            StringBuilder hexString = new StringBuilder();
            for (byte b : hash) {
                String hex = Integer.toHexString(0xff & b);
                if (hex.length() == 1) hexString.append('0');
                hexString.append(hex);
            }
            return hexString.toString();
        } catch (Exception ex) {
            return "";
        }
    }

    private int dp(int dp) {
        return (int) (dp * getResources().getDisplayMetrics().density);
    }

    private GradientDrawable createRoundedBg(int color, int radius) {
        GradientDrawable gd = new GradientDrawable();
        gd.setColor(color);
        gd.setCornerRadius(radius);
        return gd;
    }

    private LinearLayout createCard() {
        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setBackground(createRoundedBg(cardColor, dp(16)));
        card.setPadding(dp(16), dp(16), dp(16), dp(16));
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        params.setMargins(0, 0, 0, dp(10));
        card.setLayoutParams(params);
        return card;
    }

    private void appendLog(String msg) {
        uiHandler.post(() -> {
            if (logView != null) {
                logView.append("\n> " + msg);
                if (logView.getParent() instanceof ScrollView) {
                    ((ScrollView) logView.getParent()).post(() -> ((ScrollView) logView.getParent()).fullScroll(View.FOCUS_DOWN));
                }
            }
        });
    }

    private String runSu(String cmd) {
        try {
            ProcessBuilder pb = new ProcessBuilder("su", "-mm", "-c", cmd);
            pb.redirectErrorStream(true);
            Process p = pb.start();
            BufferedReader r = new BufferedReader(new InputStreamReader(p.getInputStream()));
            StringBuilder sb = new StringBuilder();
            String line;
            while ((line = r.readLine()) != null) sb.append(line).append("\n");
            p.waitFor();
            return sb.toString().trim();
        } catch (Exception e) { return "ERROR: " + e.getMessage(); }
    }

    private void extractAsset(String name, File outFile) {
        try (InputStream is = getAssets().open(name); FileOutputStream fos = new FileOutputStream(outFile)) {
            byte[] buf = new byte[8192]; int len;
            while ((len = is.read(buf)) > 0) fos.write(buf, 0, len);
        } catch (Exception e) { 
            appendLog("ОШИБКА " + name + ": " + e.getMessage()); 
        }
    }

    private void startProcess() {
        try {
            if (!runSu("id -u").equals("0")) { appendLog("Нет рут-прав!"); return; }

            appendLog("Подготовка...");
            String runtime = "/data/local/tmp/hantoo-local";
            runSu("mkdir -p " + runtime + " && chmod 700 " + runtime);

            File Dir = getCacheDir();
            File libMain = new File(Dir, "libmain.so");
            File libInjector = new File(Dir, "libinjector.so");
            File startSh = new File(Dir, "start.sh");
            File stopSh = new File(Dir, "stop.sh");
            File restSh = new File(Dir, "");
            File Bin = new File(Dir, "");

            extractAsset("libmain.so", libMain);
            extractAsset("libinjector.so", libInjector);
            extractAsset("start.sh", startSh);
            extractAsset("stop.sh", stopSh);
            extractAsset("", restSh);
            extractAsset("", Bin);

            runSu("cp " + libMain.getAbsolutePath() + " " + runtime + "/libmain.so");
            runSu("cp " + libInjector.getAbsolutePath() + " " + runtime + "/manualmap_injector");
            runSu("cp " + startSh.getAbsolutePath() + " " + runtime + "/start.sh");
            runSu("cp " + stopSh.getAbsolutePath() + " " + runtime + "/stop.sh");
            runSu("cp " + restSh.getAbsolutePath() + " " + runtime + "/");
            runSu("cp " + Bin.getAbsolutePath() + " " + runtime + "/");

            runSu("chmod 700 " + runtime + "/manualmap_injector " + runtime + "/start.sh " + runtime + "/stop.sh " + runtime + "/");
            runSu("chmod 600 " + runtime + "/libmain.so " + runtime + "/");

            appendLog("Запуск start.sh...");
            
            String cmd = "cd " + runtime + " && sh start.sh";
            ProcessBuilder pb = new ProcessBuilder("su", "-mm", "-c", cmd);
            pb.redirectErrorStream(true);
            currentProcess = pb.start();
            
            BufferedReader r = new BufferedReader(new InputStreamReader(currentProcess.getInputStream()));
            String line;
            while ((line = r.readLine()) != null) {
                appendLog(line);
            }
            int exitCode = currentProcess.waitFor();
            if (exitCode == 0) {
                appendLog("Успешно. Чит в игре!");
                uiHandler.post(() -> {
                    statusText.setText("● Running");
                    statusText.setTextColor(colorGreen);
                });
            } else {
                appendLog("Ошибка скрипта (код " + exitCode + ").");
                uiHandler.post(() -> statusText.setText("○ Stopped"));
            }

        } catch (Throwable e) {
            appendLog("Ошибка: " + e.getMessage());
            uiHandler.post(() -> statusText.setText("○ Error"));
        }
    }

    private void stopProcess() {
        executor.submit(() -> {
            appendLog("Остановка...");
            String runtime = "/data/local/tmp/hantoo-local";
            runSu("cd " + runtime + " && sh stop.sh");
            if (currentProcess != null) currentProcess.destroy();
            appendLog("Остановлено.");
            uiHandler.post(() -> {
                statusText.setText("○ Stopped");
                statusText.setTextColor(textColorSecondary);
            });
        });
    }
}
