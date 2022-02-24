#include "Main.hpp"

#include "GlobalNamespace/ScoreModel.hpp"

#include "TMPro/TextMeshPro.hpp"

#include <sstream>
#include <iomanip>

using namespace GlobalNamespace;
using namespace HSV;

Judgement& GetBestJudgement(std::vector<Judgement>& judgements, int comparison) {
    Judgement* best = nullptr;
    for(auto& judgement : judgements) {
        if(comparison >= judgement.Threshold && (!best || judgement.Threshold > best->Threshold))
            best = &judgement;
    }
    return best ? *best : judgements.back();
}

std::string GetBestSegmentText(std::vector<Segment>& segments, int comparison) {
    Segment* best = nullptr;
    for(auto& segment : segments) {
        if(comparison >= segment.Threshold && (!best || segment.Threshold > best->Threshold))
            best = &segment;
    }
    return best ? best->Text : "";
}

std::string GetBestFloatSegmentText(std::vector<FloatSegment>& segments, float comparison) {
    FloatSegment* best = nullptr;
    for(auto& segment : segments) {
        if(comparison >= segment.Threshold && (!best || segment.Threshold > best->Threshold))
            best = &segment;
    }
    return best ? best->Text : "";
}

std::string TimeDependenceString(float timeDependence) {
    int multiplier = std::pow(10, globalConfig.CurrentConfig->TimeDependenceDecimalOffset);
    std::stringstream ss;
    ss << std::fixed << std::setprecision(globalConfig.CurrentConfig->TimeDependenceDecimalPrecision) << (timeDependence * multiplier);
    return ss.str();
}

std::string GetJudgementText(Judgement& judgement, int score, int before, int after, int accuracy, float timeDependence) {
    auto& text = judgement.Text;

    text.set_beforeCut(std::to_string(before));
    text.set_accuracy(std::to_string(accuracy));
    text.set_afterCut(std::to_string(after));
    text.set_score(std::to_string(score));
    text.set_percent(std::to_string(std::round(100 * (float) score / 115)));
    text.set_timeDependency(TimeDependenceString(timeDependence));
    text.set_beforeCutSegment(GetBestSegmentText(globalConfig.CurrentConfig->BeforeCutAngleSegments, before));
    text.set_accuracySegment(GetBestSegmentText(globalConfig.CurrentConfig->AccuracySegments, accuracy));
    text.set_afterCutSegment(GetBestSegmentText(globalConfig.CurrentConfig->AfterCutAngleSegments, after));
    text.set_timeDependencySegment(GetBestFloatSegmentText(globalConfig.CurrentConfig->TimeDependenceSegments, timeDependence));

    return text.Join();
}

UnityEngine::Color GetJudgementColor(Judgement& judgement, int score) {
    if(!judgement.Fade || !judgement.Fade.value())
        return judgement.Color;
    // get the lowest judgement with a higher threshold
    Judgement* best = nullptr;
    for(auto& judgement : globalConfig.CurrentConfig->Judgements) {
        if(score < judgement.Threshold && (!best || judgement.Threshold < best->Threshold))
            best = &judgement;
    }
    if(!best)
        return judgement.Color;
    int lowerThreshold = judgement.Threshold;
    int higherThreshold = best->Threshold;
    float lerpDistance = ((float) score - lowerThreshold) / (higherThreshold - lowerThreshold);
    auto lowerColor = judgement.Color;
    auto higherColor = best->Color;
    return UnityEngine::Color(
        lowerColor.r + (higherColor.r - lowerColor.r) * lerpDistance,
        lowerColor.g + (higherColor.g - lowerColor.g) * lerpDistance,
        lowerColor.b + (higherColor.b - lowerColor.b) * lerpDistance,
        lowerColor.a + (higherColor.a - lowerColor.a) * lerpDistance
    );
}

void UpdateScoreEffect(FlyingScoreEffect* flyingScoreEffect, int total, int before, int after, int accuracy, float timeDependence) {
    auto& judgement = GetBestJudgement(globalConfig.CurrentConfig->Judgements, total);

    auto text = GetJudgementText(judgement, total, before, after, accuracy, timeDependence);
    auto color = GetJudgementColor(judgement, total);

    flyingScoreEffect->text->set_text(text);
    flyingScoreEffect->text->set_color(color);
    flyingScoreEffect->color = color;
}

void Judge(ISaberSwingRatingCounter* swingCounter, FlyingScoreEffect* flyingScoreEffect, NoteCutInfo noteCutInfo) {

    int before, after, accuracy;
    ScoreModel::RawScoreWithoutMultiplier(swingCounter, noteCutInfo.cutDistanceToCenter, byref(before), byref(after), byref(accuracy));
    int total = before + after + accuracy;
    float timeDependence = std::abs(noteCutInfo.cutNormal.z);

    UpdateScoreEffect(flyingScoreEffect, total, before, after, accuracy, timeDependence);
}