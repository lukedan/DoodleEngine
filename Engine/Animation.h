#pragma once

#include "Event.h"
#include "Vector2.h"
#include "Property.h"

namespace DE {
	namespace Core {
		struct AnimationFinishInfo {
			public:
				AnimationFinishInfo() : ExceededTime(0.0) {
				}
				explicit AnimationFinishInfo(double exTime) : ExceededTime(exTime) {
				}

				ReferenceProperty<double, PropertyType::ReadOnly> ExceededTime;
		};
		class Animation {
			public:
				Animation() : Finish() {
				}
				virtual ~Animation() {
				}

				virtual void Update(double) = 0;

				double GetDuration() const {
					return _dur;
				}
				virtual void SetDuration(double newDuration) {
					_dur = newDuration;
				}

				bool Ended() const {
					return _ended;
				}
				bool Started() const {
					return _going;
				}
				virtual void Start() {
					if (!_ended) {
						_going = true;
					}
				}
				virtual void Restart() {
					_ended = false;
					_timeTot = 0.0;
				}
				virtual void Stop() {
					_going = false;
				}

				Event<AnimationFinishInfo> Finish;
			protected:
				double _timeTot = 0.0, _dur = 1.0;
				bool _going = true, _ended = false;
		};

		template <typename T> class TweenAnimation : public Animation {
			public:
				TweenAnimation() {
				}
				virtual ~TweenAnimation() {
				}

				virtual void Update(double dt) override {
					if (!_ended) {
						_timeTot += dt;
						_progress = _timeTot / _dur;
						TranslateValue(TranslateProgress());
					}
				}
				const T &GetValue() const {
					return _curVal;
				}

				const T &GetFromValue() const {
					return _start;
				}
				virtual void SetFromValue(const T &obj) {
					_start = obj;
					RecalculateDelta();
				}
				const T &GetToValue() const {
					return _end;
				}
				virtual void SetToValue(const T &obj) {
					_end = obj;
					RecalculateDelta();
				}

				double GetAccelerationRatio() const {
					return _accR;
				}
				virtual void SetAccelerationRatio(double ratio) {
					if (ratio < 0.0 || ratio + _decR > 1.0) {
						throw OverflowException(_TEXT("acceleration ratio overflow"));
					}
					_accR = ratio;
					RecalculateMaxSpeed();
				}

				double GetDecelerationRatio() const {
					return _decR;
				}
				virtual void SetDecelerationRatio(double ratio) {
					if (ratio < 0.0 || ratio + _accR > 1.0) {
						throw OverflowException(_TEXT("deceleration ratio overflow"));
					}
					_decR = ratio;
					RecalculateMaxSpeed();
				}

				virtual void Redirect(const T &newEnd) {
					_timeTot = 0.0;
					_start = _curVal;
					_end = newEnd;
					RecalculateDelta();
					RecalculateMaxSpeed();
					_going = true;
					_ended = false;
				}
			protected:
				virtual double TranslateProgress() {
					if (_progress >= 1.0) {
						_going = false;
						_ended = true;
						Finish(AnimationFinishInfo(_timeTot - _dur));
						return 1.0;
					}
					if (_progress < _accR) {
						return _maxSpd * _progress * _progress / (2 * _accR);
					}
					if (_progress <= 1.0 - _decR) {
						return _maxSpd * (_progress - _accR * 0.5);
					}
					double omp = 1.0 - _progress;
					return 1.0 - _maxSpd * omp * omp / (2 * _decR);
				}
				virtual void TranslateValue(double val) {
					_curVal = _delt * val + _start;
				}
				virtual void RecalculateMaxSpeed() {
					_maxSpd = 2.0 / (2.0 - _accR - _decR);
				}
				virtual void RecalculateDelta() {
					_delt = _end - _start;
				}

				T _start, _end, _curVal, _delt;
				double _accR = 0.0, _decR = 0.0, _maxSpd = 1.0, _progress = 0.0;
		};
		typedef TweenAnimation<double> DoubleAnimation;
		typedef TweenAnimation<float> FloatAnimation;
		typedef TweenAnimation<Math::Vector2> Vector2Animation;

		template <typename T> class TweenAnimationWithInitialSpeed : public TweenAnimation<T> {
			public:
				TweenAnimationWithInitialSpeed() = default;
				explicit TweenAnimationWithInitialSpeed(const T &initSpd) : TweenAnimation<T>(), _oriSpd(initSpd) {
				}
				virtual ~TweenAnimationWithInitialSpeed() {
				}

                const T &GetInitialSpeed() const {
                	return _oriSpd;
                }
				virtual void SetInitialSpeed(const T &in) {
					_oriSpd = in;
				}

				virtual void Redirect(const T &newEnd) override {
					Base::Redirect(newEnd);
					_oriSpd = _curSpd;
				}
			protected:
				virtual void TranslateValue(double val) override {
					/*
						v0 * t + 0.5 * a * t^2 = x
						0.5 * a * t^2 = x - v0 * t
						a = 2 * (x - v0 * t) / t^2
						x' = v0 * t' + (x - v0 * t) * t'^2 / t^2
						   = v0 * t' + x * t'^2 / t^2 - v0 * t'^2 / t
						   = v0 * (t - t') * t' / t + x * (t' / t)^2
						   = (v0 * (t - t') + x * (t' / t)) * (t' / t)
						v' = v0 + t' * 2 * (x - v0 * t) / t^2
					*/
					_curSpd = _oriSpd + 2 * val * (Base::_delt - _oriSpd);
					Base::_curVal = Base::_start + (_oriSpd * (1 - val) + Base::_delt * val) * val;
				}

				T _oriSpd, _curSpd;

				typedef TweenAnimation<T> Base;
		};
		typedef TweenAnimationWithInitialSpeed<double> DoubleAnimationWithInitialSpeed;
		typedef TweenAnimationWithInitialSpeed<float> FloatAnimationWithInitialSpeed;
		typedef TweenAnimationWithInitialSpeed<Math::Vector2> Vector2AnimationWithInitialSpeed;
	}
}
