#pragma once

namespace Roses
{
	typedef enum class KeyCode : uint16_t
	{
		// From glfw3.h
		Space               = 32,
		Apostrophe          = 39, /* ' */
		Comma               = 44, /* , */
		Minus               = 45, /* - */
		Period              = 46, /* . */
		Slash               = 47, /* / */

		D0                  = 48, /* 0 */
		D1                  = 49, /* 1 */
		D2                  = 50, /* 2 */
		D3                  = 51, /* 3 */
		D4                  = 52, /* 4 */
		D5                  = 53, /* 5 */
		D6                  = 54, /* 6 */
		D7                  = 55, /* 7 */
		D8                  = 56, /* 8 */
		D9                  = 57, /* 9 */

		Semicolon           = 59, /* ; */
		Equal               = 61, /* = */

		A                   = 65,
		B                   = 66,
		C                   = 67,
		D                   = 68,
		E                   = 69,
		F                   = 70,
		G                   = 71,
		H                   = 72,
		I                   = 73,
		J                   = 74,
		K                   = 75,
		L                   = 76,
		M                   = 77,
		N                   = 78,
		O                   = 79,
		P                   = 80,
		Q                   = 81,
		R                   = 82,
		S                   = 83,
		T                   = 84,
		U                   = 85,
		V                   = 86,
		W                   = 87,
		X                   = 88,
		Y                   = 89,
		Z                   = 90,

		LeftBracket         = 91,  /* [ */
		Backslash           = 92,  /* \ */
		RightBracket        = 93,  /* ] */
		GraveAccent         = 96,  /* ` */

		World1              = 161, /* non-US #1 */
		World2              = 162, /* non-US #2 */

		/* Function keys */
		Escape              = 256,
		Enter               = 257,
		Tab                 = 258,
		Backspace           = 259,
		Insert              = 260,
		Delete              = 261,
		Right               = 262,
		Left                = 263,
		Down                = 264,
		Up                  = 265,
		PageUp              = 266,
		PageDown            = 267,
		Home                = 268,
		End                 = 269,
		CapsLock            = 280,
		ScrollLock          = 281,
		NumLock             = 282,
		PrintScreen         = 283,
		Pause               = 284,
		F1                  = 290,
		F2                  = 291,
		F3                  = 292,
		F4                  = 293,
		F5                  = 294,
		F6                  = 295,
		F7                  = 296,
		F8                  = 297,
		F9                  = 298,
		F10                 = 299,
		F11                 = 300,
		F12                 = 301,
		F13                 = 302,
		F14                 = 303,
		F15                 = 304,
		F16                 = 305,
		F17                 = 306,
		F18                 = 307,
		F19                 = 308,
		F20                 = 309,
		F21                 = 310,
		F22                 = 311,
		F23                 = 312,
		F24                 = 313,
		F25                 = 314,

		/* Keypad */
		KP0                 = 320,
		KP1                 = 321,
		KP2                 = 322,
		KP3                 = 323,
		KP4                 = 324,
		KP5                 = 325,
		KP6                 = 326,
		KP7                 = 327,
		KP8                 = 328,
		KP9                 = 329,
		KPDecimal           = 330,
		KPDivide            = 331,
		KPMultiply          = 332,
		KPSubtract          = 333,
		KPAdd               = 334,
		KPEnter             = 335,
		KPEqual             = 336,

		LeftShift           = 340,
		LeftControl         = 341,
		LeftAlt             = 342,
		LeftSuper           = 343,
		RightShift          = 344,
		RightControl        = 345,
		RightAlt            = 346,
		RightSuper          = 347,
		Menu                = 348
	} Key;

	inline std::ostream& operator<<(std::ostream& os, KeyCode keyCode)
	{
		os << static_cast<int32_t>(keyCode);
		return os;
	}
}

// From glfw3.h
#define HZ_KEY_SPACE           ::Roses::Key::Space
#define HZ_KEY_APOSTROPHE      ::Roses::Key::Apostrophe    /* ' */
#define HZ_KEY_COMMA           ::Roses::Key::Comma         /* , */
#define HZ_KEY_MINUS           ::Roses::Key::Minus         /* - */
#define HZ_KEY_PERIOD          ::Roses::Key::Period        /* . */
#define HZ_KEY_SLASH           ::Roses::Key::Slash         /* / */
#define HZ_KEY_0               ::Roses::Key::D0
#define HZ_KEY_1               ::Roses::Key::D1
#define HZ_KEY_2               ::Roses::Key::D2
#define HZ_KEY_3               ::Roses::Key::D3
#define HZ_KEY_4               ::Roses::Key::D4
#define HZ_KEY_5               ::Roses::Key::D5
#define HZ_KEY_6               ::Roses::Key::D6
#define HZ_KEY_7               ::Roses::Key::D7
#define HZ_KEY_8               ::Roses::Key::D8
#define HZ_KEY_9               ::Roses::Key::D9
#define HZ_KEY_SEMICOLON       ::Roses::Key::Semicolon     /* ; */
#define HZ_KEY_EQUAL           ::Roses::Key::Equal         /* = */
#define HZ_KEY_A               ::Roses::Key::A
#define HZ_KEY_B               ::Roses::Key::B
#define HZ_KEY_C               ::Roses::Key::C
#define HZ_KEY_D               ::Roses::Key::D
#define HZ_KEY_E               ::Roses::Key::E
#define HZ_KEY_F               ::Roses::Key::F
#define HZ_KEY_G               ::Roses::Key::G
#define HZ_KEY_H               ::Roses::Key::H
#define HZ_KEY_I               ::Roses::Key::I
#define HZ_KEY_J               ::Roses::Key::J
#define HZ_KEY_K               ::Roses::Key::K
#define HZ_KEY_L               ::Roses::Key::L
#define HZ_KEY_M               ::Roses::Key::M
#define HZ_KEY_N               ::Roses::Key::N
#define HZ_KEY_O               ::Roses::Key::O
#define HZ_KEY_P               ::Roses::Key::P
#define HZ_KEY_Q               ::Roses::Key::Q
#define HZ_KEY_R               ::Roses::Key::R
#define HZ_KEY_S               ::Roses::Key::S
#define HZ_KEY_T               ::Roses::Key::T
#define HZ_KEY_U               ::Roses::Key::U
#define HZ_KEY_V               ::Roses::Key::V
#define HZ_KEY_W               ::Roses::Key::W
#define HZ_KEY_X               ::Roses::Key::X
#define HZ_KEY_Y               ::Roses::Key::Y
#define HZ_KEY_Z               ::Roses::Key::Z
#define HZ_KEY_LEFT_BRACKET    ::Roses::Key::LeftBracket   /* [ */
#define HZ_KEY_BACKSLASH       ::Roses::Key::Backslash     /* \ */
#define HZ_KEY_RIGHT_BRACKET   ::Roses::Key::RightBracket  /* ] */
#define HZ_KEY_GRAVE_ACCENT    ::Roses::Key::GraveAccent   /* ` */
#define HZ_KEY_WORLD_1         ::Roses::Key::World1        /* non-US #1 */
#define HZ_KEY_WORLD_2         ::Roses::Key::World2        /* non-US #2 */

/* Function keys */
#define HZ_KEY_ESCAPE          ::Roses::Key::Escape
#define HZ_KEY_ENTER           ::Roses::Key::Enter
#define HZ_KEY_TAB             ::Roses::Key::Tab
#define HZ_KEY_BACKSPACE       ::Roses::Key::Backspace
#define HZ_KEY_INSERT          ::Roses::Key::Insert
#define HZ_KEY_DELETE          ::Roses::Key::Delete
#define HZ_KEY_RIGHT           ::Roses::Key::Right
#define HZ_KEY_LEFT            ::Roses::Key::Left
#define HZ_KEY_DOWN            ::Roses::Key::Down
#define HZ_KEY_UP              ::Roses::Key::Up
#define HZ_KEY_PAGE_UP         ::Roses::Key::PageUp
#define HZ_KEY_PAGE_DOWN       ::Roses::Key::PageDown
#define HZ_KEY_HOME            ::Roses::Key::Home
#define HZ_KEY_END             ::Roses::Key::End
#define HZ_KEY_CAPS_LOCK       ::Roses::Key::CapsLock
#define HZ_KEY_SCROLL_LOCK     ::Roses::Key::ScrollLock
#define HZ_KEY_NUM_LOCK        ::Roses::Key::NumLock
#define HZ_KEY_PRINT_SCREEN    ::Roses::Key::PrintScreen
#define HZ_KEY_PAUSE           ::Roses::Key::Pause
#define HZ_KEY_F1              ::Roses::Key::F1
#define HZ_KEY_F2              ::Roses::Key::F2
#define HZ_KEY_F3              ::Roses::Key::F3
#define HZ_KEY_F4              ::Roses::Key::F4
#define HZ_KEY_F5              ::Roses::Key::F5
#define HZ_KEY_F6              ::Roses::Key::F6
#define HZ_KEY_F7              ::Roses::Key::F7
#define HZ_KEY_F8              ::Roses::Key::F8
#define HZ_KEY_F9              ::Roses::Key::F9
#define HZ_KEY_F10             ::Roses::Key::F10
#define HZ_KEY_F11             ::Roses::Key::F11
#define HZ_KEY_F12             ::Roses::Key::F12
#define HZ_KEY_F13             ::Roses::Key::F13
#define HZ_KEY_F14             ::Roses::Key::F14
#define HZ_KEY_F15             ::Roses::Key::F15
#define HZ_KEY_F16             ::Roses::Key::F16
#define HZ_KEY_F17             ::Roses::Key::F17
#define HZ_KEY_F18             ::Roses::Key::F18
#define HZ_KEY_F19             ::Roses::Key::F19
#define HZ_KEY_F20             ::Roses::Key::F20
#define HZ_KEY_F21             ::Roses::Key::F21
#define HZ_KEY_F22             ::Roses::Key::F22
#define HZ_KEY_F23             ::Roses::Key::F23
#define HZ_KEY_F24             ::Roses::Key::F24
#define HZ_KEY_F25             ::Roses::Key::F25

/* Keypad */
#define HZ_KEY_KP_0            ::Roses::Key::KP0
#define HZ_KEY_KP_1            ::Roses::Key::KP1
#define HZ_KEY_KP_2            ::Roses::Key::KP2
#define HZ_KEY_KP_3            ::Roses::Key::KP3
#define HZ_KEY_KP_4            ::Roses::Key::KP4
#define HZ_KEY_KP_5            ::Roses::Key::KP5
#define HZ_KEY_KP_6            ::Roses::Key::KP6
#define HZ_KEY_KP_7            ::Roses::Key::KP7
#define HZ_KEY_KP_8            ::Roses::Key::KP8
#define HZ_KEY_KP_9            ::Roses::Key::KP9
#define HZ_KEY_KP_DECIMAL      ::Roses::Key::KPDecimal
#define HZ_KEY_KP_DIVIDE       ::Roses::Key::KPDivide
#define HZ_KEY_KP_MULTIPLY     ::Roses::Key::KPMultiply
#define HZ_KEY_KP_SUBTRACT     ::Roses::Key::KPSubtract
#define HZ_KEY_KP_ADD          ::Roses::Key::KPAdd
#define HZ_KEY_KP_ENTER        ::Roses::Key::KPEnter
#define HZ_KEY_KP_EQUAL        ::Roses::Key::KPEqual

#define HZ_KEY_LEFT_SHIFT      ::Roses::Key::LeftShift
#define HZ_KEY_LEFT_CONTROL    ::Roses::Key::LeftControl
#define HZ_KEY_LEFT_ALT        ::Roses::Key::LeftAlt
#define HZ_KEY_LEFT_SUPER      ::Roses::Key::LeftSuper
#define HZ_KEY_RIGHT_SHIFT     ::Roses::Key::RightShift
#define HZ_KEY_RIGHT_CONTROL   ::Roses::Key::RightControl
#define HZ_KEY_RIGHT_ALT       ::Roses::Key::RightAlt
#define HZ_KEY_RIGHT_SUPER     ::Roses::Key::RightSuper
#define HZ_KEY_MENU            ::Roses::Key::Menu
