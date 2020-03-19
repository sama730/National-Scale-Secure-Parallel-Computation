#ifndef GATE_H__
#define GATE_H__

#pragma once

#include <vector>
#include <string>

namespace Circuit
{
	/// <summary>
	/// Interface to Denote a logical gate.
	/// </summary>
	class IGate
	{
	public:
	    virtual ~IGate(){};
	    std::string ToString(){};
	};

	/// <summary>
	/// Interface to denote a gate with a single input.
	/// </summary>
	class UnitaryGate : public IGate
	{
	public:
		int InputWire;
		int OutputWire;

		UnitaryGate(int inputWire, int outputWire);
		UnitaryGate(const UnitaryGate &obj);
		UnitaryGate(const IGate &obj);
		
		std::string ToString();
		~UnitaryGate(){};
	};

	/// <summary>
	/// Interface to denote an arithmetic gate with two inputs.
	/// </summary>
	class BinaryGate : public IGate
	{
	public:
		int LeftWire;
		int RightWire;
		int OutputWire;

		/// <summary>
		/// Abstract Instantiation an arithmetic gate.
		/// </summary>
		BinaryGate(int LeftWire, int RightWire, int OutputWire);
		BinaryGate(const BinaryGate &obj);
		BinaryGate(const IGate &obj);
		
		std::string ToString();
		~BinaryGate(){};
	};
	
	class NaryGate : public IGate
	{
	public:
		std::vector<int> InputWires;
		int OutputWire;
		
		NaryGate(std::vector<int> inputWires, int outputWire);
		std::string ToString();
		~NaryGate(){};
	};
}

#endif
