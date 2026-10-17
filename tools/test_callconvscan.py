"""Tests for callconvscan.py, on hand-written prologues and declarations.

    uv run tools/test_callconvscan.py
"""

import unittest

import capstone

import callconvscan as cs


def decode(code: bytes):
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    return list(md.disasm(code, 0x401000))


class ReadsEcxFirst(unittest.TestCase):
    def test_this_use(self):
        self.assertTrue(cs.reads_ecx_first(decode(bytes.fromhex("8b01c3"))))

    def test_ecx_set_first(self):
        self.assertFalse(cs.reads_ecx_first(decode(bytes.fromhex("b90500000089c8"))))

    def test_or_minus_one_is_a_write(self):
        self.assertFalse(cs.reads_ecx_first(decode(bytes.fromhex("83c9ff89c8"))))

    def test_push_ecx_reserves_a_slot(self):
        self.assertFalse(cs.reads_ecx_first(decode(bytes.fromhex("5133c9"))))

    def test_edx(self):
        self.assertTrue(cs.reads_ecx_first(decode(bytes.fromhex("89d0c3")), cs.EDX))


class Patterns(unittest.TestCase):
    def test_member_decoration(self):
        self.assertTrue(cs.MEMBER_DECORATION.match("?Close@MoviePlayer@@QAEXXZ"))
        self.assertTrue(cs.MEMBER_DECORATION.match("?F@A@B@@UAEHXZ"))
        self.assertFalse(cs.MEMBER_DECORATION.match("?DrawString@@YGXPAUSurface@@PAEHHH@Z"))
        self.assertFalse(cs.MEMBER_DECORATION.match("?Make@Foo@@SGXXZ"))

    def test_declaration(self):
        self.assertEqual(cs.DECL.match("int __stdcall Foo(void* p);").group(2), "Foo")
        self.assertEqual(cs.DECL.match("    void __fastcall Bar(int);").group(2), "Bar")
        self.assertIsNone(cs.DECL.match("typedef int (__stdcall *Fn)(int);"))
        self.assertIsNone(cs.DECL.match("int (__stdcall *g_fn)(int);"))


if __name__ == "__main__":
    unittest.main()
