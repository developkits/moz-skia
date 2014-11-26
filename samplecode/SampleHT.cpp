/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SampleCode.h"
#include "SkView.h"
#include "SkCanvas.h"
#include "SkCanvasDrawable.h"
#include "SkInterpolator.h"
#include "SkPictureRecorder.h"
#include "SkRandom.h"

const SkRect gUnitSquare = { -1, -1, 1, 1 };

static void color_to_floats(SkColor c, SkScalar f[4]) {
    f[0] = SkIntToScalar(SkColorGetA(c));
    f[1] = SkIntToScalar(SkColorGetR(c));
    f[2] = SkIntToScalar(SkColorGetG(c));
    f[3] = SkIntToScalar(SkColorGetB(c));
}

static SkColor floats_to_color(const SkScalar f[4]) {
    return SkColorSetARGB(SkScalarRoundToInt(f[0]),
                          SkScalarRoundToInt(f[1]),
                          SkScalarRoundToInt(f[2]),
                          SkScalarRoundToInt(f[3]));
}

static bool oval_contains(const SkRect& r, SkScalar x, SkScalar y) {
    SkMatrix m;
    m.setRectToRect(r, gUnitSquare, SkMatrix::kFill_ScaleToFit);
    SkPoint pt;
    m.mapXY(x, y, &pt);
    return pt.lengthSqd() <= 1;
}

static SkColor rand_opaque_color(uint32_t seed) {
    SkRandom rand(seed);
    return rand.nextU() | (0xFF << 24);
}

class HTDrawable : public SkCanvasDrawable {
    SkRect          fR;
    SkColor         fColor;
    SkInterpolator* fInterp;

public:
    HTDrawable(SkRandom& rand) {
        fR = SkRect::MakeXYWH(rand.nextRangeF(0, 640), rand.nextRangeF(0, 480),
                              rand.nextRangeF(20, 200), rand.nextRangeF(20, 200));
        fColor = rand_opaque_color(rand.nextU());
        fInterp = NULL;
    }
    
    void spawnAnimation() {
        SkDELETE(fInterp);
        fInterp = SkNEW_ARGS(SkInterpolator, (5, 3));
        SkScalar values[5];
        color_to_floats(fColor, values); values[4] = 0;
        fInterp->setKeyFrame(0, SampleCode::GetAnimTime(), values);
        values[0] = 0; values[4] = 180;
        fInterp->setKeyFrame(1, SampleCode::GetAnimTime() + 1000, values);
        color_to_floats(rand_opaque_color(fColor), values); values[4] = 360;
        fInterp->setKeyFrame(2, SampleCode::GetAnimTime() + 2000, values);

        fInterp->setMirror(true);
        fInterp->setRepeatCount(3);

        this->notifyDrawingChanged();
    }

    bool hitTest(SkScalar x, SkScalar y) {
        return oval_contains(fR, x, y);
    }

    void onDraw(SkCanvas* canvas) SK_OVERRIDE {
        SkAutoCanvasRestore acr(canvas, false);

        SkPaint paint;
        paint.setAntiAlias(true);

        if (fInterp) {
            SkScalar values[5];
            SkInterpolator::Result res = fInterp->timeToValues(SampleCode::GetAnimTime(), values);
            fColor = floats_to_color(values);

            canvas->save();
            canvas->translate(fR.centerX(), fR.centerY());
            canvas->rotate(values[4]);
            canvas->translate(-fR.centerX(), -fR.centerY());

            switch (res) {
                case SkInterpolator::kFreezeEnd_Result:
                    SkDELETE(fInterp);
                    fInterp = NULL;
                    break;
                default:
                    break;
            }
        }
        paint.setColor(fColor);
        canvas->drawRect(fR, paint);
    }
    
    SkRect onGetBounds() SK_OVERRIDE { return fR; }
};

class HTView : public SampleView {
public:
    enum {
        N = 50,
        W = 640,
        H = 480,
    };
    
    struct Rec {
        HTDrawable* fDrawable;
    };
    Rec fArray[N];
    SkAutoTUnref<SkCanvasDrawable> fRoot;
    
    HTView() {
        SkRandom rand;

        SkPictureRecorder recorder;
        SkCanvas* canvas = recorder.beginRecording(SkRect::MakeWH(W, H));
        for (int i = 0; i < N; ++i) {
            fArray[i].fDrawable = new HTDrawable(rand);
            canvas->EXPERIMENTAL_drawDrawable(fArray[i].fDrawable);
            fArray[i].fDrawable->unref();
        }
        fRoot.reset(recorder.EXPERIMENTAL_endRecordingAsDrawable());
    }

protected:
    bool onQuery(SkEvent* evt) SK_OVERRIDE {
        if (SampleCode::TitleQ(*evt)) {
            SampleCode::TitleR(evt, "HT");
            return true;
        }
        return this->INHERITED::onQuery(evt);
    }

    void onDrawContent(SkCanvas* canvas) SK_OVERRIDE {
        canvas->EXPERIMENTAL_drawDrawable(fRoot);
        this->inval(NULL);
    }

    SkView::Click* onFindClickHandler(SkScalar x, SkScalar y, unsigned modi) SK_OVERRIDE {
        // search backwards to find the top-most
        for (int i = N - 1; i >= 0; --i) {
            if (fArray[i].fDrawable->hitTest(x, y)) {
                fArray[i].fDrawable->spawnAnimation();
                break;
            }
        }
        this->inval(NULL);
        return NULL;
    }

private:
    typedef SampleView INHERITED;
};

//////////////////////////////////////////////////////////////////////////////

static SkView* MyFactory() { return new HTView; }
static SkViewRegister reg(MyFactory);
