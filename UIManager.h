// �ɮ�: UIManager.h
#pragma once
#include <windows.h>
#include <string>
#include <vector>

// �V�M�ת���L�����ŧi UI ��l�ƨ禡
void PrepareUI();

// �ŧi�������ܼơA�H�K��L .cpp �ɮץi�H�s��
extern HFONT g_hCustomFont;  // �Ω�ۭq���s���r��

// �i�ץ��j�ŧi�s���r������N�X�A�Ϩ��b dllmain.cpp ���Q�s��
extern HFONT g_hSnapButtonFont;
extern HFONT g_hPoemLabelFont;