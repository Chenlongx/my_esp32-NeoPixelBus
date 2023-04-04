// NeoPixelFunLoop
// 此示例将在一系列像素周围移动一条光线。
// 环形像素看起来最好。
// 拖尾将具有缓慢褪色的尾巴。
//
// 这将演示使用NeoPixelAnimator的用法。
// 它显示了使用动画来控制其他动画的修改和启动的高级用法。
// 它还展示了动画颜色的正常使用。
// 它还演示了共享动画通道的能力，而不是将其硬编码到像素中。
//
//

#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>


const uint16_t PixelCount = 256;  // 确保将其设置为带中的像素数量
const uint16_t PixelPin = 2;  // 确保将其设置为正确的引脚，在Esp8266中忽略
const uint16_t AnimCount = PixelCount / 5 * 2 + 1;  // 我们只需要足够的动画来制作拖尾和一个额外的动画

const uint16_t PixelFadeDuration = 300; // 三分之一秒
// 每秒分割为像素数=每秒循环一次
const uint16_t NextPixelMoveDuration = 1000 / PixelCount; // 我们通过像素移动的速度

NeoGamma<NeoGammaTableMethod> colorGamma; // 对于任何褪色动画，最好进行gamma矫正

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);
// 对于Esp8266，引脚被省略，它使用GPIO3由于DMA硬件使用。
// 还有其他提供更多引脚选项但也具有其他副作用的Esp8266替代方法。
// 有关详细信息，请参见此处链接的wiki https://github.com/Makuna/NeoPixelBus/wiki/ESP8266-NeoMethods

// 状态存储是根据需要指定的，在此示例中为颜色和
// 动画的像素; 基本上是您在动画更新函数中所需的任何内容
struct MyAnimationState
{
    RgbColor StartingColor;
    RgbColor EndingColor;
    uint16_t IndexPixel;  // 此动画影响的像素
};

NeoPixelAnimator animations(AnimCount); // NeoPixel animation management object
MyAnimationState animationState[AnimCount];
uint16_t frontPixel = 0;  // the front of the loop
RgbColor frontColor;  // the color at the front of the loop

void SetRandomSeed()
{
    uint32_t seed;

    // random works best with a seed that can use 31 bits
    // analogRead on a unconnected pin tends toward less than four bits
    seed = analogRead(0);
    delay(1);

    for (int shifts = 3; shifts < 31; shifts += 3)
    {
        seed ^= analogRead(0) << shifts;
        delay(1);
    }

    // Serial.println(seed);
    randomSeed(seed);
}

void FadeOutAnimUpdate(const AnimationParam& param)
{
    // this gets called for each animation on every time step
    // progress will start at 0.0 and end at 1.0
    // we use the blend function on the RgbColor to mix
    // color based on the progress given to us in the animation
    RgbColor updatedColor = RgbColor::LinearBlend(
        animationState[param.index].StartingColor,
        animationState[param.index].EndingColor,
        param.progress);
    // apply the color to the strip
    strip.SetPixelColor(animationState[param.index].IndexPixel,
        colorGamma.Correct(updatedColor));
}

void LoopAnimUpdate(const AnimationParam& param)
{
    // wait for this animation to complete,
    // we are using it as a timer of sorts
    if (param.state == AnimationState_Completed)
    {
        // done, time to restart this position tracking animation/timer
        animations.RestartAnimation(param.index);

        // pick the next pixel inline to start animating
        //
        frontPixel = (frontPixel + 1) % PixelCount; // increment and wrap
        if (frontPixel == 0)
        {
            // we looped, lets pick a new front color
            frontColor = HslColor(random(360) / 360.0f, 1.0f, 0.25f);
        }

        uint16_t indexAnim;
        // 我们是否有可用于动画下一个前端像素的动画？
        // 如果看到跳跃，则可能是您的速度过快或需要增加
        // 动画通道的数量
        if (animations.NextAvailableAnimation(&indexAnim, 1))
        {
            animationState[indexAnim].StartingColor = frontColor;
            animationState[indexAnim].EndingColor = RgbColor(0, 0, 0);
            animationState[indexAnim].IndexPixel = frontPixel;

            animations.StartAnimation(indexAnim, PixelFadeDuration, FadeOutAnimUpdate);
        }
    }
}

void setup()
{
    strip.Begin();
    strip.Show();

    SetRandomSeed();

    // 我们使用索引0的动画来计时何时移动到灯带上的下一个像素
    animations.StartAnimation(0, NextPixelMoveDuration, LoopAnimUpdate);
}


void loop()
{
    // 这是保持运行所需的全部内容，
    // 避免使用 delay() 对于任何与时间相关的例程都是一件好事。
    animations.UpdateAnimations();
    strip.Show();
}



