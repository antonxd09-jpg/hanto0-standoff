package com.hanto0.loader;
import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.view.Gravity;
import android.view.WindowManager;
import android.webkit.JavascriptInterface;
import android.webkit.WebView;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;

public class MainActivity extends Activity {
    private WindowManager wm;
    private WebView wv;
    private WindowManager.LayoutParams lp;

    @Override
    protected void onCreate(Bundle s) {
        super.onCreate(s);
        LinearLayout r = new LinearLayout(this);
        r.setOrientation(LinearLayout.VERTICAL);
        r.setGravity(Gravity.CENTER);
        r.setBackgroundColor(Color.parseColor("#0d0d0f"));
        TextView tv = new TextView(this);
        tv.setText("HantoO v2.4.1 — Standoff 2 (50s Root Auto-Launcher)");
        tv.setTextColor(Color.WHITE);
        tv.setGravity(Gravity.CENTER);
        r.addView(tv);
        Button b = new Button(this);
        b.setText("START");
        b.setOnClickListener(v -> {
            if (Build.VERSION.SDK_INT >= 23 && !Settings.canDrawOverlays(this)) {
                startActivity(new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION, Uri.parse("package:" + getPackageName())));
                return;
            }
            b.setEnabled(false);
            tv.setText("Starting Standoff 2... Menu will appear in 50s");
            runRootFlow();
        });
        r.addView(b);
        setContentView(r);
    }

    private void runRootFlow() {
        new Thread(() -> {
            try {
                String d = getApplicationInfo().nativeLibraryDir;
                Runtime.getRuntime().exec(new String[]{"su", "-c",
                    "cp -f " + d + "/libhanto0.so /data/local/tmp/libhanto0.so && " +
                    "cp -f " + d + "/libinjector.so /data/local/tmp/injector && " +
                    "chmod 755 /data/local/tmp/injector /data/local/tmp/libhanto0.so && " +
                    "monkey -p com.axlebolt.standoff2 -c android.intent.category.LAUNCHER 1"
                }).waitFor();
            } catch (Exception ignored) {}
        }).start();

        new Handler(Looper.getMainLooper()).postDelayed(() -> {
            new Thread(() -> {
                try {
                    Runtime.getRuntime().exec(new String[]{"su", "-c",
                        "/data/local/tmp/injector com.axlebolt.standoff2 /data/local/tmp/libhanto0.so"
                    }).waitFor();
                } catch (Exception ignored) {}
            }).start();
            spawnMenu();
        }, 50000);
    }

    private void spawnMenu() {
        wm = (WindowManager) getSystemService(WINDOW_SERVICE);
        int t = Build.VERSION.SDK_INT >= 26 ? WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY : WindowManager.LayoutParams.TYPE_PHONE;
        lp = new WindowManager.LayoutParams(-1, -1, t, 8 | 256, PixelFormat.TRANSLUCENT);
        lp.gravity = Gravity.TOP | Gravity.START;
        wv = new WebView(getApplicationContext());
        wv.setBackgroundColor(Color.TRANSPARENT);
        wv.getSettings().setJavaScriptEnabled(true);
        wv.addJavascriptInterface(new Bridge(), "HantoBridge");
        wv.loadUrl("file:///android_asset/menu.html");
        wm.addView(wv, lp);
    }

    public class Bridge {
        @JavascriptInterface public void sendCommand(String c) { ipc(c); }
        @JavascriptInterface public String fetchEsp() {
            String r = ipc("GET_ESP");
            return (r == null || r.isEmpty()) ? "{\"players\":[]}" : r;
        }
        @JavascriptInterface public void setMenuExpanded(boolean e) {
            new Handler(Looper.getMainLooper()).post(() -> {
                float d = getResources().getDisplayMetrics().density;
                lp.width = e ? -1 : (int)(32 * d);
                wm.updateViewLayout(wv, lp);
            });
        }
        private String ipc(String p) {
            try (LocalSocket s = new LocalSocket()) {
                s.connect(new LocalSocketAddress("hanto0_so2_ipc", LocalSocketAddress.Namespace.ABSTRACT));
                s.setSoTimeout(150);
                OutputStream os = s.getOutputStream();
                os.write(p.getBytes(StandardCharsets.UTF_8));
                os.flush();
                InputStream is = s.getInputStream();
                byte[] b = new byte[8192];
                int n = is.read(b);
                if (n > 0) return new String(b, 0, n, StandardCharsets.UTF_8);
            } catch (Exception ignored) {}
            return "";
        }
    }
}
