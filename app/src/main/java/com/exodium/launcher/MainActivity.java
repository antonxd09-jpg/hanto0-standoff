package com.exodium.launcher;

import android.app.Activity;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;
import java.io.*;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class MainActivity extends Activity {
    private final ExecutorService executor = Executors.newSingleThreadExecutor();
    private final Handler uiHandler = new Handler(Looper.getMainLooper());
    private TextView logView;
    private ScrollView scrollView;
    private Process currentProcess;
    private TextView statusText;

    private final int bgColor = Color.parseColor("#0F1115");
    private final int cardColor = Color.parseColor("#1A1D24");
    private final int textColorPrimary = Color.parseColor("#FFFFFF");
    private final int textColorSecondary = Color.parseColor("#8E929A");
    private final int accentColor = Color.parseColor("#8C9EFF");
    private final int accentColorDark = Color.parseColor("#232738");
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
            layout.setPadding(dp(20), dp(30), dp(20), dp(30));

            TextView topBar = new TextView(this);
            topBar.setText("HANTOO / STANDOFF 2");
            topBar.setTextColor(accentColor);
            topBar.setTextSize(12);
            topBar.setTypeface(Typeface.MONOSPACE);
            layout.addView(topBar);

            TextView title = new TextView(this);
            title.setText("ESP & Aimbot");
            title.setTextColor(textColorPrimary);
            title.setTextSize(32);
            title.setTypeface(Typeface.DEFAULT_BOLD);
            title.setPadding(0, dp(5), 0, dp(2));
            layout.addView(title);

            TextView subtitle = new TextView(this);
            subtitle.setText("Memory · Overlay · ROOT required");
            subtitle.setTextColor(textColorSecondary);
            subtitle.setTextSize(13);
            subtitle.setPadding(0, 0, 0, dp(20));
            layout.addView(subtitle);

            LinearLayout tabs = new LinearLayout(this);
            tabs.setOrientation(LinearLayout.HORIZONTAL);
            tabs.setPadding(0, 0, 0, dp(20));
            
            tabs.addView(createTab("GENERAL", true));
            tabs.addView(createTab("VISUALS", false));
            tabs.addView(createTab("SETTINGS", false));
            layout.addView(tabs);

            LinearLayout statusCard = createCard();
            TextView statusTitle = new TextView(this);
            statusTitle.setText("Status");
            statusTitle.setTextColor(textColorPrimary);
            statusTitle.setTextSize(16);
            statusTitle.setTypeface(Typeface.DEFAULT_BOLD);
            
            TextView statusDesc = new TextView(this);
            statusDesc.setText("Launch Standoff 2, enter a match, then start.");
            statusDesc.setTextColor(textColorSecondary);
            statusDesc.setTextSize(13);
            statusDesc.setPadding(0, dp(5), 0, dp(10));

            statusText = new TextView(this);
            statusText.setText("○ Stopped");
            statusText.setTextColor(textColorSecondary);
            statusText.setTextSize(14);
            statusText.setTypeface(Typeface.MONOSPACE);

            statusCard.addView(statusTitle);
            statusCard.addView(statusDesc);
            statusCard.addView(statusText);
            layout.addView(statusCard);

            // Кнопка START (Зеленая)
            Button startBtn = new Button(this);
            startBtn.setText("START");
            startBtn.setTextColor(Color.WHITE);
            startBtn.setTextSize(16);
            startBtn.setTypeface(Typeface.DEFAULT_BOLD);
            startBtn.setBackground(createRoundedBg(colorGreen, dp(12)));
            
            LinearLayout.LayoutParams btnParams = new LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT, dp(55));
            btnParams.setMargins(0, dp(20), 0, dp(15));
            startBtn.setLayoutParams(btnParams);
            startBtn.setOnClickListener(v -> {
                statusText.setText("○ Starting...");
                statusText.setTextColor(accentColor);
                executor.submit(this::startProcess);
            });
            layout.addView(startBtn);

            // Кнопка STOP (Красная)
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

            LinearLayout infoCard = createCard();
            TextView infoTitle = new TextView(this);
            infoTitle.setText("Info");
            infoTitle.setTextColor(textColorPrimary);
            infoTitle.setTextSize(16);
            infoTitle.setTypeface(Typeface.DEFAULT_BOLD);
            
            TextView infoDesc = new TextView(this);
            infoDesc.setText("ROOT + SYSTEM_ALERT_WINDOW required.");
            infoDesc.setTextColor(textColorSecondary);
            infoDesc.setTextSize(13);
            infoDesc.setPadding(0, dp(5), 0, 0);

            infoCard.addView(infoTitle);
            infoCard.addView(infoDesc);
            layout.addView(infoCard);

            TextView logLabel = new TextView(this);
            logLabel.setText("SYSTEM LOGS");
            logLabel.setTextColor(textColorSecondary);
            logLabel.setTextSize(11);
            logLabel.setTypeface(Typeface.MONOSPACE);
            logLabel.setPadding(0, dp(20), 0, dp(5));
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

    private TextView createTab(String text, boolean isActive) {
        TextView tab = new TextView(this);
        tab.setText(text);
        tab.setTextSize(12);
        tab.setTypeface(Typeface.DEFAULT_BOLD);
        tab.setGravity(Gravity.CENTER);
        tab.setPadding(dp(15), dp(8), dp(15), dp(8));
        
        int bg = isActive ? accentColorDark : Color.TRANSPARENT;
        int textColor = isActive ? accentColor : textColorSecondary;
        
        tab.setBackground(createRoundedBg(bg, dp(10)));
        tab.setTextColor(textColor);
        
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f);
        params.setMargins(dp(4), 0, dp(4), 0);
        tab.setLayoutParams(params);
        
        return tab;
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

            String nativeDir = getApplicationInfo().nativeLibraryDir;
            runSu("cp " + nativeDir + "/libinjector.so " + runtime + "/manualmap_injector");
            runSu("cp " + nativeDir + "/libmain.so " + runtime + "/libmain.so");

            File cacheDir = getCacheDir();
            File startSh = new File(cacheDir, "start.sh");
            File stopSh = new File(cacheDir, "stop.sh");
            File restSh = new File(cacheDir, "rest.sh");
            File cacheBin = new File(cacheDir, "cache");

            extractAsset("start.sh", startSh);
            extractAsset("stop.sh", stopSh);
            extractAsset("rest.sh", restSh);
            extractAsset("cache", cacheBin);

            runSu("cp " + startSh.getAbsolutePath() + " " + runtime + "/start.sh");
            runSu("cp " + stopSh.getAbsolutePath() + " " + runtime + "/stop.sh");
            runSu("cp " + restSh.getAbsolutePath() + " " + runtime + "/rest.sh");
            runSu("cp " + cacheBin.getAbsolutePath() + " " + runtime + "/cache");

            runSu("chmod 700 " + runtime + "/manualmap_injector " + runtime + "/start.sh " + runtime + "/stop.sh " + runtime + "/rest.sh");
            runSu("chmod 600 " + runtime + "/libmain.so " + runtime + "/cache");

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
