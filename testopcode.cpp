#define CLOVE_SUITE_NAME MySuite01
#include <iostream>
#include <vector>
#include <random>
#include <cstdint>
#include "clove-unit.h"
#include "main.cpp"

using namespace chipotto;

std::vector<uint16_t> generateRandomHexArray(int count)
{
    std::vector<uint16_t> hexArray(count);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint16_t> dis(0x1000, 0x1FFF);

    for (auto& hexValue : hexArray)
    {
        hexValue = dis(gen);
    }
    return hexArray;
}

CLOVE_TEST(testOpcode0) {
    // 00EE - RET
    chipotto::Emulator emulator;

    uint16_t input = 0xEE;
    uint16_t Sp = emulator.GetSP();

    emulator.Opcode0(input);
    std::array<uint16_t, 0x10> array1 = emulator.GetStack();

    CLOVE_INT_EQ(emulator.GetPC() ,array1[Sp & 0xF]);
    CLOVE_INT_EQ(emulator.GetSP(), Sp-1);
}

CLOVE_TEST(testOpcode1) {
    // 1nnn - JP addr
    chipotto::Emulator emulator;

    uint16_t input = 0x1F87;
    uint16_t expected = input & 0x0FFF;
    emulator.Opcode1(input);
    
    CLOVE_INT_EQ(emulator.GetPC(),expected-2);
}

CLOVE_TEST(testOpcode1WithRandomValues) {
    // 1nnn - JP addr
    chipotto::Emulator emulator;
    uint16_t expected;

    int count = 10;
    std::vector<uint16_t> hexValues = generateRandomHexArray(count);
    for (const auto& value : hexValues)
    {
        expected = value & 0x0FFF;
        emulator.Opcode1(value);
        CLOVE_INT_EQ(emulator.GetPC(), expected - 2);
    }
}

CLOVE_TEST(testOpcode2) {
    // 2nnn - CALL addr
    chipotto::Emulator emulator;

    uint16_t input = 0x2FF8;
    uint16_t Sp = emulator.GetSP()+1;
    uint16_t PreviusPc = emulator.GetPC();
  
    emulator.Opcode2(input);
    std::array<uint16_t, 0x10> localStack = emulator.GetStack();
   
    CLOVE_INT_EQ(localStack[Sp & 0xF], PreviusPc);
    CLOVE_INT_EQ(emulator.GetPC(), input & 0xFFF);
}

CLOVE_TEST(testOpcode3) {
    // 3xkk - SE Vx, byte
    chipotto::Emulator emulator;
   
    uint16_t input = 0x3FE5;

    // branch taken 
    uint16_t PreviousPC = emulator.GetPC();
    emulator.SetValueInRegisters(0xF,0xE5);
    emulator.Opcode3(input);
    
    CLOVE_INT_EQ(emulator.GetPC(), PreviousPC+2);

    // branch not taken 
    PreviousPC = emulator.GetPC();
    emulator.SetValueInRegisters(0xF, 0xE7);
    emulator.Opcode3(input);

    CLOVE_INT_NE(emulator.GetPC(), PreviousPC + 2);   
}

CLOVE_TEST(testOpcode4) {
    // 4xkk - SNE Vx, byte
    chipotto::Emulator emulator;
    uint16_t input = 0x4FE5;

    // branch not taken 
    uint16_t PreviousPC = emulator.GetPC();
    emulator.SetValueInRegisters(0xF, 0xE32);
    emulator.Opcode4(input);

    CLOVE_INT_EQ(emulator.GetPC(), PreviousPC+2);

    // branch taken 
    PreviousPC = emulator.GetPC();
    emulator.SetValueInRegisters(0xF, 0xE5);
    emulator.Opcode4(input);

    CLOVE_INT_NE(emulator.GetPC(), PreviousPC+2);
}

CLOVE_TEST(testOpcode6) {
    // 6xkk - LD Vx, byte
    chipotto::Emulator emulator;
    uint16_t input = 0x6FE5;
    emulator.Opcode6(input);
    std::array<uint8_t, 0x10> localRegisters = emulator.GetRegisters();
    CLOVE_INT_EQ(localRegisters[0xF], 0xE5);
}

CLOVE_TEST(testOpcode7) {
    // 7xkk - ADD Vx, byte
    chipotto::Emulator emulator;
    uint16_t input = 0x7FE5;
    
    uint8_t prev = emulator.GetRegisters().at(0xF);

    emulator.Opcode7(input);

    CLOVE_INT_EQ(emulator.GetRegisters().at(0xF), prev+0xE5);
}

CLOVE_TEST(testOpcode8) {
    // 8xy0 - LD Vx, Vy --- Set Vx = Vy.
    chipotto::Emulator emulator;
    uint16_t input = 0x8D30;

    emulator.SetValueInRegisters(0x3, 0xF);

    emulator.Opcode8(input);
    uint8_t registerX = emulator.GetRegisters().at(0xD);
    uint8_t registerY = emulator.GetRegisters().at(0x3);

    CLOVE_INT_EQ(registerX, registerY);
    
    // 8xy2 - AND Vx, Vy --- AND Vx, Vy
    input = 0x8D32;
    // set Vx
    emulator.SetValueInRegisters(0xD, 0xC);
    // set Vy
    emulator.SetValueInRegisters(0x3, 0x3);

    emulator.Opcode8(input);

    registerX = emulator.GetRegisters().at(0xD);
    registerY = emulator.GetRegisters().at(0x3);

    
    CLOVE_INT_EQ(registerX, registerX & registerY);
    
    // 8xy3 - XOR Vx, Vy 
    input = 0x8DC3;
    // set Vx
    emulator.SetValueInRegisters(0xD, 0xC);
    uint8_t prevVx = emulator.GetRegisters().at(0xD);
    // set Vy
    emulator.SetValueInRegisters(0xC, 0x3);

    emulator.Opcode8(input);

    registerX = emulator.GetRegisters().at(0xD);
    registerY = emulator.GetRegisters().at(0xC);

    CLOVE_INT_EQ(registerX, prevVx ^ registerY);
    
    // 8xy4 - ADD Vx, Vy --- case < 8 bit
    input = 0x8DC4;

    // set Vx
    emulator.SetValueInRegisters(0xD, 0xC);
    prevVx = emulator.GetRegisters().at(0xD);
    // set Vy
    emulator.SetValueInRegisters(0xC, 0x3);

    emulator.Opcode8(input);

    registerX = emulator.GetRegisters().at(0xD);
    registerY = emulator.GetRegisters().at(0xC);
    uint8_t registerF = emulator.GetRegisters().at(0xF);
  
    CLOVE_INT_EQ(registerX, prevVx + registerY);
    CLOVE_INT_EQ(registerF, 0x0);

    // 8xy4 - ADD Vx, Vy --- case > 8 bit
    input = 0x8DC4;

    // set Vx
    emulator.SetValueInRegisters(0xD, 0x80);
    prevVx = emulator.GetRegisters().at(0xD);
    // set Vy
    emulator.SetValueInRegisters(0xC, 0x82);

    emulator.Opcode8(input);

    registerX = emulator.GetRegisters().at(0xD);
    registerY = emulator.GetRegisters().at(0xC);
    registerF = emulator.GetRegisters().at(0xF);

    CLOVE_INT_EQ(registerX, (prevVx + registerY) & 0xFF);
    CLOVE_INT_EQ(registerF, 0x1);

    // 8xyE - SHL Vx {, Vy} --- case VF=1
    input = 0x8DCE;
    emulator.SetValueInRegisters(0xD, 0xFC);
    prevVx = emulator.GetRegisters().at(0xD);
    emulator.Opcode8(input);

    registerX = emulator.GetRegisters().at(0xD);
    registerF = emulator.GetRegisters().at(0xF);
    CLOVE_INT_EQ(registerX, (uint8_t)0x1F8);
    CLOVE_INT_EQ(registerF, 0x1);

    // 8xyE - SHL Vx {, Vy} --- case VF=0
    input = 0x8DCE;
    emulator.SetValueInRegisters(0xD, 0x2);
    prevVx = emulator.GetRegisters().at(0xD);
    emulator.Opcode8(input);

    registerX = emulator.GetRegisters().at(0xD);
    registerF = emulator.GetRegisters().at(0xF);
    CLOVE_INT_EQ(registerX, 0x4);
    CLOVE_INT_EQ(registerF, 0x0);
}

CLOVE_TEST(testOpcodeA) {
    // Annn - LD I, addr
    chipotto::Emulator emulator;
    uint16_t input = 0xADFC;

    emulator.OpcodeA(input);

    CLOVE_INT_EQ(emulator.GetI(),0xDFC);
}

CLOVE_TEST(testOpcodeC) {
    // Cxkk - RND Vx, byte
    chipotto::Emulator emulator;
    uint16_t input = 0xCD00;

    emulator.OpcodeC(input);

    uint8_t registerX = emulator.GetRegisters().at(0xD);
    CLOVE_INT_EQ(registerX, 0x0);
}

CLOVE_TEST(testOpcodeF) {
    // Fx1E - ADD I, Vx
    chipotto::Emulator emulator;
    uint16_t input = 0xFD1E;

    emulator.SetValueInRegisters(0xD, 0x2);
    emulator.SetI(0xC);

    emulator.OpcodeF(input);

    CLOVE_INT_EQ(emulator.GetI(), 0xE);
}


