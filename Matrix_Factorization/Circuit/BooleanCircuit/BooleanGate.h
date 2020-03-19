#ifndef BOOLEAN_GATE_H__
#define BOOLEAN_GATE_H__

#pragma once

#include "Gate.h"


namespace Circuit
{
	/// <summary>
	/// Interface to Denote a logical gate.
	/// </summary>
	class IBooleanGate : public IGate
	{
	public:
		~IBooleanGate(){}
	};
	
	/// <summary>
	/// Used to represent an AND-gate.
	/// </summary>
	class AndGate : public BinaryGate //, public IBooleanGate
	{
		/// <summary>
		/// Creates new Boolean Gate
		/// </summary>
	public:
		AndGate(int LeftWire, int RightWire, int OutputWire);
	};

	/// <summary>
	/// Used to represent an XOR-gate.
	/// </summary>
	class XorGate : public BinaryGate//, public IBooleanGate
	{
		/// <summary>
		/// Create new Xor Gate.
		/// </summary>
	public:
		XorGate(int LeftWire, int RightWire, int OutputWire);
	};

	/// <summary>
	/// Interface to denote a negation gate.
	/// </summary>
	class NotGate : public UnitaryGate//, public IBooleanGate
	{
		/// <summary>
		/// Returns a not gate.
		/// </summary>
		/// <param name="inputWire"></param>
		/// <param name="outputWire"></param>
	public:
		NotGate(int inputWire, int outputWire);
	};
}

#endif
