#include "DataFiles.h"

namespace trioio::library
{

juce::File trioioDataFile(const juce::String& relative)
{
    juce::Array<juce::File> candidates;
    if (const auto env = juce::SystemStats::getEnvironmentVariable("TRIOIO_DATA_DIR", {}); env.isNotEmpty())
        candidates.add(juce::File(env).getChildFile(relative));
    const auto exeDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
    candidates.add(exeDir.getChildFile("data").getChildFile(relative));
    candidates.add(exeDir.getParentDirectory().getChildFile("Resources").getChildFile("data").getChildFile(relative));
    candidates.add(exeDir.getParentDirectory().getChildFile("share").getChildFile("trioio").getChildFile("data").getChildFile(relative));
#ifdef TRIOIO_SOURCE_DIR
    const juce::File source = juce::File(juce::String(TRIOIO_SOURCE_DIR)).getChildFile("data").getChildFile(relative);
    candidates.add(source);
#else
    const juce::File source = candidates.getLast();
#endif
    for (const auto& f : candidates)
        if (f.existsAsFile())
            return f;
    return source;
}

} // namespace trioio::library
