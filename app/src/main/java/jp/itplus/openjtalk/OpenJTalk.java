package jp.itplus.openjtalk;

import android.support.annotation.Keep;

import java.io.Closeable;
import java.io.File;

public class OpenJTalk implements Closeable {

    static {
        System.loadLibrary("open-jtalk");
    }

    //-----------------------------------------------------------------
    //  Instance variables
    //-----------------------------------------------------------------

    @Keep
    long instance;

    //-----------------------------------------------------------------
    //  Constructors
    //-----------------------------------------------------------------

    public OpenJTalk() {
        instance = nativeInit();
    }

    @Override
    public void finalize() throws Throwable {
        super.finalize();
        close();
    }

    @Override
    public void close() {
        nativeDestroy(instance);
    }

    //-----------------------------------------------------------------
    //  Properties
    //-----------------------------------------------------------------

    public int getSamplingFrequency() {
        return nativeGetSamplingFrequency(instance);
    }

    public void setSamplingFrequency(int value) {
        nativeSetSamplingFrequency(instance, value);
    }

    public double getAlpha() {
        return nativeGetAlpha(instance);
    }

    public void setAlpha(double value) {
        nativeSetAlpha(instance, value);
    }

    public double getBeta() {
        return nativeGetBeta(instance);
    }

    public void setBeta(double value) {
        nativeSetBeta(instance, value);
    }

    public void setSpeed(double value) {
        nativeSetSpeed(instance, value);
    }

    public void addHalfTone(double value) {
        nativeAddHalfTone(instance, value);
    }

    public double getMsdThreshold(int index) {
        return nativeGetMsdThreshold(instance, index);
    }

    public void setMsdThreshold(int index, double value) {
        nativeSetMsdThreshold(instance, index, value);
    }

    public double getGvWeight(int index) {
        return nativeGetGvWeight(instance, index);
    }

    public void setGvWeight(int index, double value) {
        nativeSetGvWeight(instance, index, value);
    }

    public double getVolume() {
        return nativeGetVolume(instance);
    }

    public void setVolume(double volume) {
        nativeSetVolume(instance, volume);
    }

    public int getAudioBufferSize() {
        return nativeGetAudioBufferSize(instance);
    }

    public void setAudioBufferSize(int value) {
        nativeSetAudioBufferSize(instance, value);
    }

    //-----------------------------------------------------------------
    //  Operations
    //-----------------------------------------------------------------

    public boolean load(File mecabDir, File voiceFile) {
        return nativeLoad(instance, mecabDir.getAbsolutePath(), voiceFile.getAbsolutePath());
    }

    public boolean talk(String text) {
        return talk(text, null, null);
    }

    public boolean talk(String text, File waveFile, File logFile) {
        return nativeTalk(instance, text, (waveFile != null) ? waveFile.getAbsolutePath() : null, (logFile != null) ? logFile.getAbsolutePath() : null);
    }

    //-----------------------------------------------------------------
    //  natives
    //-----------------------------------------------------------------

    private native static long nativeInit();

    private native static void nativeDestroy(long instance);

    private native static int nativeGetSamplingFrequency(long instance);

    private native static void nativeSetSamplingFrequency(long instance, int freq);

    private native static double nativeGetAlpha(long instance);

    private native static void nativeSetAlpha(long instance, double alpha);

    private native static double nativeGetBeta(long instance);

    private native static void nativeSetBeta(long instance, double beta);

    private native static void nativeSetSpeed(long instance, double speed);

    private native static void nativeAddHalfTone(long instance, double value);

    private native static double nativeGetMsdThreshold(long instance, int index);

    private native static void nativeSetMsdThreshold(long instance, int index, double value);

    private native static double nativeGetGvWeight(long instance, int index);

    private native static void nativeSetGvWeight(long instance, int index, double value);

    private native static double nativeGetVolume(long instance);

    private native static void nativeSetVolume(long instance, double value);

    private native static int nativeGetAudioBufferSize(long instance);

    private native static void nativeSetAudioBufferSize(long instance, int value);

    private native static boolean nativeLoad(long instance, String dirMecab, String fnVoice);

    private native static boolean nativeTalk(long instance, String text, String waveFile, String logFile);
}
