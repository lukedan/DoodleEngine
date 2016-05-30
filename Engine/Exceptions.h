#pragma once

namespace DE {
	namespace Core {
		class Exception {
			public:
				Exception() = default;
				explicit Exception(const TCHAR *message) : _msg(message) {
				}
				virtual ~Exception() {
				}

				const TCHAR *Message() const {
					return _msg;
				}
			protected:
				const TCHAR *_msg = nullptr;
		};

		class OverflowException : public Exception {
			public:
				explicit OverflowException(const TCHAR *message) : Exception(message) {
				}
		};
		class UnderflowException : public Exception {
			public:
				explicit UnderflowException(const TCHAR *message) : Exception(message) {
				}
		};
		class SystemException : public Exception {
			public:
				explicit SystemException(const TCHAR *message) : Exception(message) {
				}
		};
		class InvalidOperationException : public Exception {
			public:
				explicit InvalidOperationException(const TCHAR *message) : Exception(message) {
				}
		};
		class InvalidArgumentException : public Exception {
			public:
				explicit InvalidArgumentException(const TCHAR *message) : Exception(message) {
				}
		};
	}
}
