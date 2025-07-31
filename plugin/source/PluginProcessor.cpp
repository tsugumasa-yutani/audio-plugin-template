#include "YourPluginName/PluginProcessor.h"
#include "YourPluginName/PluginEditor.h"

namespace audio_plugin {

// パラメータID
constexpr auto cutoffId = "cutoff";
constexpr auto qId = "q";
constexpr auto filterTypeId = "filterType";
// AudioProcessorValueTreeStateの初期化
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
              ),
      apvts(*this,
            nullptr,
            juce::Identifier("Params"),
            {std::make_unique<juce::AudioParameterFloat>(
                 cutoffId,
                 "Cutoff",
                 juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.5f),
                 1000.0f),
             std::make_unique<juce::AudioParameterFloat>(
                 qId,
                 "Q",
                 juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f),
                 1.0f),
             std::make_unique<juce::AudioParameterChoice>(
                 filterTypeId,
                 "Filter Type",
                 juce::StringArray{"BandPass", "LowPass"},
                 0)}) {
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor() {}

const juce::String AudioPluginAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool AudioPluginAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const {
  return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms() {
  return 1;  // NB: some hosts don't cope very well if you tell them there are 0
             // programs, so this should be at least 1, even if you're not
             // really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram() {
  return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram(int index) {
  juce::ignoreUnused(index);
}

const juce::String AudioPluginAudioProcessor::getProgramName(int index) {
  juce::ignoreUnused(index);
  return {};
}

void AudioPluginAudioProcessor::changeProgramName(int index,
                                                  const juce::String& newName) {
  juce::ignoreUnused(index, newName);
}

void AudioPluginAudioProcessor::prepareToPlay(double sampleRate,
                                              int samplesPerBlock) {
  // 再生開始時やサンプルレート変更時に呼ばれる初期化処理
  juce::ignoreUnused(sampleRate, samplesPerBlock);
  size_t numChannels = static_cast<size_t>(getTotalNumInputChannels());
  xHistory.assign(numChannels, {0.0f, 0.0f});
  yHistory.assign(numChannels, {0.0f, 0.0f});
  // パラメータキャッシュを初期化
  prevCutoff = -1.0f;
  prevQ = -1.0f;
  prevFilterType = -1;
}

void AudioPluginAudioProcessor::releaseResources() {
  // When playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const {
#if JucePlugin_IsMidiEffect
  juce::ignoreUnused(layouts);
  return true;
#else
  // This is the place where you check if the layout is supported.
  // In this template code we only support mono or stereo.
  // Some plugin hosts, such as certain GarageBand versions, will only
  // load plugins that support stereo bus layouts.
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;
#endif

  return true;
#endif
}

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages) {
  juce::ignoreUnused(midiMessages);

  juce::ScopedNoDenormals noDenormals;
  size_t totalNumInputChannels =
      static_cast<size_t>(getTotalNumInputChannels());
  size_t totalNumOutputChannels =
      static_cast<size_t>(getTotalNumOutputChannels());
  float sampleRate = static_cast<float>(getSampleRate());

  // --- ここからフィルタ係数の計算 ---
  // パラメータ値を取得
  float cutoffFreq = *apvts.getRawParameterValue(cutoffId);
  float Q = *apvts.getRawParameterValue(qId);
  int filterType = static_cast<int>(*apvts.getRawParameterValue(filterTypeId));

  // パラメータが変化したときのみ係数を再計算（安全な浮動小数点比較を使用）
  constexpr float epsilon = 1e-4f;  // 比較の許容誤差
  bool isCutoffChanged = std::abs(cutoffFreq - prevCutoff) > epsilon;
  bool isQChanged = std::abs(Q - prevQ) > epsilon;
  bool isFilterTypeChanged = filterType != prevFilterType;

  if (isCutoffChanged || isQChanged || isFilterTypeChanged) {
    float omegaC = 2.0f * static_cast<float>(M_PI) * cutoffFreq / sampleRate;
    float K = std::tan(omegaC / 2.0f);
    if (filterType == 0) {  // BandPass
      float norm = 1.0f / (Q + K + Q * K * K);
      filterB[0] = K * norm;
      filterB[1] = 0.0f;
      filterB[2] = -K * norm;
      filterA[0] = 1.0f;
      filterA[1] = (-2.0f * Q + 2.0f * Q * K * K) * norm;
      filterA[2] = (Q - K + Q * K * K) * norm;
    } else if (filterType == 1) {  // LowPass
      float norm = 1.0f / (1.0f + K / Q + K * K);
      filterB[0] = K * K * norm;
      filterB[1] = 2.0f * K * K * norm;
      filterB[2] = K * K * norm;
      filterA[0] = 1.0f;
      filterA[1] = 2.0f * (K * K - 1.0f) * norm;
      filterA[2] = (1.0f - K / Q + K * K) * norm;
    }
    prevCutoff = cutoffFreq;
    prevQ = Q;
    prevFilterType = filterType;
  }

  // In case we have more outputs than inputs, this code clears any output
  // channels that didn't contain input data, (because these aren't
  // guaranteed to be empty - they may contain garbage).
  // This is here to avoid people getting screaming feedback
  // when they first compile a plugin, but obviously you don't need to keep
  // this code if your algorithm always overwrites all the output channels.
  for (size_t i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(static_cast<int>(i), 0, buffer.getNumSamples());

  // This is the place where you'd normally do the guts of your plugin's
  // audio processing...
  // Make sure to reset the state if your inner loop is processing
  // the samples and the outer loop is handling the channels.
  // Alternatively, you can process the samples with the channels
  // interleaved by keeping the same state.
  // --- ステレオバイクアッドフィルタ処理 ---
  for (size_t channel = 0; channel < totalNumInputChannels; ++channel) {
    float* channelData = buffer.getWritePointer(static_cast<int>(channel));
    int numSamples = buffer.getNumSamples();

    // チャンネルごとの状態変数を取得
    float x1 = xHistory[channel][0];
    float x2 = xHistory[channel][1];
    float y1 = yHistory[channel][0];
    float y2 = yHistory[channel][1];

    for (int n = 0; n < numSamples; ++n) {
      float x0 = channelData[n];
      // Difference Equation: y[n] = (b0*x[n] + b1*x[n-1] + b2*x[n-2] -
      // a1*y[n-1] - a2*y[n-2]) / a0
      float y0 = (filterB[0] * x0 + filterB[1] * x1 + filterB[2] * x2 -
                  filterA[1] * y1 - filterA[2] * y2) /
                 filterA[0];
      channelData[n] = y0;
      // 状態を更新
      x2 = x1;
      x1 = x0;
      y2 = y1;
      y1 = y0;
    }
    // 計算後の状態を保存
    xHistory[channel][0] = x1;
    xHistory[channel][1] = x2;
    yHistory[channel][0] = y1;
    yHistory[channel][1] = y2;
  }
}

// エディタ（UI）があるかどうか
bool AudioPluginAudioProcessor::hasEditor() const {
  return true;
}

// パラメータスライダーを自動生成する汎用UIを返す
juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor() {
  return new juce::GenericAudioProcessorEditor(*this);
}

void AudioPluginAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData) {
  // You should use this method to store your parameters in the memory block.
  // You could do that either as raw data, or use the XML or ValueTree classes
  // as intermediaries to make it easy to save and load complex data.
  juce::ignoreUnused(destData);
}

void AudioPluginAudioProcessor::setStateInformation(const void* data,
                                                    int sizeInBytes) {
  // You should use this method to restore your parameters from this memory
  // block, whose contents will have been created by the getStateInformation()
  // call.
  juce::ignoreUnused(data, sizeInBytes);
}
}  // namespace audio_plugin

// This creates new instances of the plugin.
// This function definition must be in the global namespace.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new audio_plugin::AudioPluginAudioProcessor();
}
