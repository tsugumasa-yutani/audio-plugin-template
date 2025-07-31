#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace audio_plugin {
class AudioPluginAudioProcessor : public juce::AudioProcessor {
public:
  AudioPluginAudioProcessor();
  ~AudioPluginAudioProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  using AudioProcessor::processBlock;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override;

  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String& newName) override;

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

public:
  // パラメータ管理用のメンバ変数（JUCE Generic
  // UIで自動認識されるようapvtsに変更）
  juce::AudioProcessorValueTreeState apvts;

private:
  // ステレオバイクアッド用の状態変数（各チャンネルごとに2つずつ）
  // xHistory: 入力履歴 [チャンネル][2], yHistory: 出力履歴 [チャンネル][2]
  std::vector<std::array<float, 2>> xHistory;
  std::vector<std::array<float, 2>> yHistory;

  // 前回のパラメータ値とフィルタ係数を保存する変数
  // prevCutoff, prevQ, prevFilterType: 前回のパラメータ値
  // filterB, filterA: フィルタ係数
  float prevCutoff = -1.0f;
  float prevQ = -1.0f;
  int prevFilterType = -1;
  float filterB[3] = {0.0f, 0.0f, 0.0f};
  float filterA[3] = {0.0f, 0.0f, 0.0f};
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};
}  // namespace audio_plugin
