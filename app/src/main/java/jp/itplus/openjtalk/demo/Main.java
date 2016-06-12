package jp.itplus.openjtalk.demo;

import android.app.Activity;
import android.app.Fragment;
import android.app.FragmentTransaction;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import jp.itplus.openjtalk.OpenJTalk;
import jp.itplus.openjtalk.R;

public final class Main {

    private Main() {
    }

    private static final boolean DEBUG_TALK = false;
    private static final String BUNDLE_KEY_MESSAGE = "msg";
    private static final String BUNDLE_KEY_DETAILS = "details";
    private static final int SAMPLING_FREQUENCY = 48000;
    private static final int AUDIO_BUFFER_SIZE = 320;


    public interface TalkInterface {
        void talk(String msg);

        void setTalkFinishedListener(TalkFinishedListener listener);
    }

    public interface TalkFinishedListener {
        void onTalkFinished(boolean success);
    }

    enum State {
        INITIALIZING,
        INITIALIZE_ERROR,
        IDLE;
    }

    public static class MainActivity extends Activity implements Handler.Callback, TalkInterface {

        private enum Messages {
            INITIALIZE,
            TALK,

            UI_INITIALIZED,
            UI_TALK_FINISHED;

            int value() {
                return ordinal();
            }

            static Messages valueOf(int code) {
                Messages[] values = Messages.values();
                if (code >= 0 && code < values.length)
                    return values[code];
                return null;
            }
        }

        private HandlerThread thread;
        private Handler handler;
        private Handler ui;
        private OpenJTalk jtalk;
        private State status;
        private Exception error;
        private TalkFinishedListener talkFinished;

        @Override
        public void onCreate(Bundle state) {
            super.onCreate(state);
            thread = new HandlerThread(MainActivity.class.getName());
            thread.start();
            handler = new Handler(thread.getLooper(), this);
            handler.obtainMessage(Messages.INITIALIZE.value()).sendToTarget();
            ui = new Handler(Looper.getMainLooper(), this);
            setStatus(State.INITIALIZING);
        }

        @Override
        public void onDestroy() {
            super.onDestroy();
            handler.getLooper().quitSafely();
        }

        @Override
        public boolean handleMessage(Message msg) {
            Messages code = Messages.valueOf(msg.what);
            if (code == null)
                return false;
            switch (code) {
                case INITIALIZE:
                    doInitialize();
                    return true;
                case TALK:
                    doTalk((String) msg.obj);
                    return true;
                case UI_INITIALIZED:
                    doInitialized(msg.arg1 != 0, (Exception) msg.obj);
                    return true;
                case UI_TALK_FINISHED:
                    doTalkFinished(msg.arg1 != 0);
                    return true;
                default:
                    break;
            }
            return false;
        }

        @Override
        public void talk(String text) {
            handler.obtainMessage(Messages.TALK.value(), text).sendToTarget();
        }

        @Override
        public void setTalkFinishedListener(TalkFinishedListener listener) {
            this.talkFinished = listener;
        }


        private void doInitialize() {
            PackageInfo info;
            try {
                info = getPackageManager().getPackageInfo(getPackageName(), 0);
            } catch (PackageManager.NameNotFoundException e) {
                notifyInitialized(false, e);
                return;
            }
            SharedPreferences prefs = getSharedPreferences("main", 0);
            long lastUpdate = prefs.getLong("LAST_UPDATE_TIME", 0);
            File dir = new File(getFilesDir(), "dict");
            File mecab = new File(dir, "mecab");
            File voice = new File(dir, "voice");
            if (info.lastUpdateTime != lastUpdate) {
                mecab.mkdirs();
                voice.mkdirs();
                AssetManager am = getAssets();
                try {
                    copyFile(mecab, am, "mecab", "char.bin");
                    copyFile(mecab, am, "mecab", "matrix.bin");
                    copyFile(mecab, am, "mecab", "sys.dic");
                    copyFile(mecab, am, "mecab", "unk.dic");
                    copyFile(voice, am, "voice", "nitech_jp_atr503_m001.htsvoice");
                    prefs.edit().putLong("LAST_UPDATE_TIME", info.lastUpdateTime);
                } catch (IOException e) {
                    notifyInitialized(false, e);
                    return;
                }
            }
            jtalk = new OpenJTalk();
            boolean success = jtalk.load(mecab, new File(voice, "nitech_jp_atr503_m001.htsvoice"));
            if (success) {
                jtalk.setSamplingFrequency(SAMPLING_FREQUENCY);
                jtalk.setAudioBufferSize(AUDIO_BUFFER_SIZE);
            }
            notifyInitialized(success, null);
        }

        private void notifyInitialized(boolean success, Exception e) {
            ui.obtainMessage(Messages.UI_INITIALIZED.value(), success ? 1 : 0, 0, e).sendToTarget();
        }

        private void doInitialized(boolean success, Exception e) {
            this.error = e;
            if (success)
                setStatus(State.IDLE);
            else
                setStatus(State.INITIALIZE_ERROR);
        }

        private void doTalk(String text) {
            if (DEBUG_TALK) {
                File dir = new File(getFilesDir(), "log");
                dir.mkdirs();
                boolean success = jtalk.talk(text, new File(dir, "wave.riff"), new File(dir, "log.txt"));
                ui.obtainMessage(Messages.UI_TALK_FINISHED.value(), success ? 1 : 0, 0).sendToTarget();
            } else {
                boolean success = jtalk.talk(text);
                ui.obtainMessage(Messages.UI_TALK_FINISHED.value(), success ? 1 : 0, 0).sendToTarget();
            }
        }

        private void doTalkFinished(boolean success) {
            if (talkFinished != null)
                talkFinished.onTalkFinished(success);
        }

        private void setStatus(State status) {
            Fragment fragment = null;
            Bundle args = null;
            this.status = status;
            switch (status) {
                case INITIALIZING:
                    fragment = new LoaderFragment();
                    break;
                case INITIALIZE_ERROR:
                    args = new Bundle();
                    args.putString(BUNDLE_KEY_MESSAGE, getString(R.string.message_initialize_error));
                    if (error != null)
                        args.putString(BUNDLE_KEY_DETAILS, error.toString());
                    fragment = new ErrorFragment();
                    break;
                case IDLE:
                    fragment = new MainFragment();
                    break;
            }
            if (fragment != null) {
                FragmentTransaction t = getFragmentManager().beginTransaction();
                if (args != null)
                    fragment.setArguments(args);
                t.replace(android.R.id.content, fragment);
                t.commit();
            }
        }

        private void copyFile(File dest, AssetManager am, String dir, String name) throws IOException {
            byte[] buffer = new byte[2048];
            FileOutputStream os = new FileOutputStream(new File(dest, name));
            InputStream is = am.open(dir + File.separator + name);
            try {
                for (; ; ) {
                    int n = is.read(buffer);
                    if (n < 1)
                        break;
                    os.write(buffer, 0, n);
                }
            } finally {
                os.close();
                is.close();
            }
        }
    }

    public static class LoaderFragment extends Fragment {

        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle state) {
            return inflater.inflate(R.layout.loading, container, false);
        }
    }

    public static class MainFragment extends Fragment implements View.OnClickListener, TalkFinishedListener, TextWatcher {

        EditText text;
        TalkInterface talk;
        TextView error;
        Button button;
        ProgressBar progress;

        @Override
        public void onAttach(Activity context) {
            super.onAttach(context);
            if (context instanceof TalkInterface)
                talk = (TalkInterface) context;
        }

        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle state) {
            View view = inflater.inflate(R.layout.main, container, false);
            text = (EditText) view.findViewById(R.id.field_text);
            text.addTextChangedListener(this);
            error = (TextView) view.findViewById(R.id.field_error);
            button = (Button) view.findViewById(R.id.button_talk);
            button.setOnClickListener(this);
            progress = (ProgressBar) view.findViewById(R.id.progress);
            progress.setVisibility(View.GONE);
            return view;
        }

        @Override
        public void onStop() {
            super.onStop();
            if (talk != null)
                talk.setTalkFinishedListener(null);
        }

        @Override
        public void onClick(View v) {
            if (talk != null) {
                setEnabled(false);
                error.setText(null);
                talk.setTalkFinishedListener(this);
                talk.talk(text.getText().toString());
            }
        }

        @Override
        public void onTalkFinished(boolean success) {
            talk.setTalkFinishedListener(null);
            error.setText(getText(success ? R.string.message_talk_success : R.string.message_talk_error));
            setEnabled(true);
        }

        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) {
        }

        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) {
        }

        @Override
        public void afterTextChanged(Editable s) {
            error.setText(null);
        }

        private void setEnabled(boolean enabled) {
            button.setEnabled(enabled);
            text.setEnabled(enabled);
            progress.setVisibility(enabled ? View.GONE : View.VISIBLE);
        }
    }

    public static class ErrorFragment extends Fragment {

        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle state) {
            View view = inflater.inflate(R.layout.error, container, false);
            TextView error = (TextView) view.findViewById(R.id.field_error);
            TextView details = (TextView) view.findViewById(R.id.field_details);
            Bundle args = getArguments();
            error.setText(args.getString(BUNDLE_KEY_MESSAGE));
            details.setText(args.getString(BUNDLE_KEY_DETAILS));
            return view;
        }
    }
}
