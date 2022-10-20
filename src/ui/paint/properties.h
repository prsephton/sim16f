/*
 * properties.h
 *
 *  Created on: 01 Oct 2022
 *      Author: paul
 *
 *  Defines property classes to be used as mix-ins with symbols, extending the
 *  functions of symbols to include the defined property as well as context
 *  menu functionality
 */

#pragma once
#include "dlg_context.h"

namespace app {
	namespace prop {


		class Rotation : virtual public Configurable{
			double m_rotation = 0;
			virtual bool needs_orientation(double &r) { r = get_rotation(); return true; }
			virtual void set_orientation(double r) { set_rotation(r); }
		  public:
			double get_rotation() const { return m_rotation; }
			void set_rotation(double r) { m_rotation = r; }
		};

		class Dimensions : virtual public Configurable{
			double m_w;
			double m_h;
			virtual bool needs_width(double &w) { w = width(); return true; }
			virtual bool needs_height(double &h) { h = width(); return true; }
			virtual void set_width(double w) { width(w); }
			virtual void set_height(double h) { height(h); }
		  public:

			double width() { return m_w; }
			void width(double h) { m_w = h; }
			double height() { return m_h; }
			void height(double h) { m_h = h; }
		};

		class Scale : virtual public Configurable{
			double m_scale = 0;
			virtual bool needs_scale(double &s) { s = get_scale(); return true; }
		  public:
			virtual void set_scale(double s) { m_scale = s; ; }
			double get_scale() const { return m_scale; }
		};

		class Inverted : virtual public Configurable{
			bool m_inverted = false;
			virtual bool needs_inverted(bool &a_inverted){ a_inverted = inverted(); return true; }
			virtual void set_inverted(bool a_inverted){ inverted(a_inverted); }
		  public:
			void inverted(bool a_invert) { m_inverted = a_invert; }
			bool inverted() const { return m_inverted; }
		};

		class Inputs: virtual public Configurable {
			int m_inputs = 0;
			virtual bool needs_inputs(int &a_inputs){a_inputs = inputs(); return true; }
			virtual void set_inputs(int a_inputs){ inputs(a_inputs); }
		  public:
			int inputs() const { return m_inputs; }
			void inputs(int a_inputs) { m_inputs = a_inputs; }
		};

//		class Gates: virtual public Configurable {
//			int m_gates = 0;
//			virtual bool needs_gates(int &a_gates){a_gates = gates(); return true; }
//			virtual void set_gates(int a_gates){ gates(a_gates); }
//		  public:
//			int gates() const { return m_gates; }
//			void gates(int a_gates) { m_gates = a_gates; }
//		};

		class Xor: virtual public Configurable {
			bool m_xor = false;

			virtual bool needs_xor(bool &a_xor){a_xor = is_xor(); return true; }

		  public:
			virtual void set_xor(int a_xor){ m_xor = a_xor; }
			bool is_xor() const { return m_xor; }
		};

		class Voltage : virtual public Configurable {
			double m_voltage = 5;

			// context menu integration
			virtual bool needs_voltage(double &a_voltage){ a_voltage = voltage(); return true; }
			virtual void set_voltage(double a_voltage){ voltage(a_voltage); }

		  public:
			double voltage() const { return m_voltage; }
			void voltage(double a_voltage) { m_voltage = a_voltage; }
		};

		class Resistance : virtual public Configurable {
			double m_resistance = 1.0;

			// context menu integration
			virtual bool needs_resistance(double &a_resistance){ a_resistance = resistance(); return true; }
			virtual void set_resistance(double a_resistance){ resistance(a_resistance); }
		  public:
			double resistance() const { return m_resistance; }
			void resistance(double a_resistance) {m_resistance = a_resistance; }
		};

		class Capacitance : virtual public Configurable {
			double m_capacitance = 1.0e-6;

			// context menu integration
			virtual bool needs_capacitance(double &a_capacitance){ a_capacitance = capacitance(); return true; }
			virtual void set_capacitance(double a_capacitance){ capacitance(a_capacitance); }
		  public:
			double capacitance() { return m_capacitance; }
			void capacitance(double a_capacitance) { m_capacitance = a_capacitance; }
		};

		class Inductance : virtual public Configurable {
			double m_inductance = 1.0e-3;

			// context menu integration
			virtual bool needs_inductance(double &a_inductance){ a_inductance = inductance(); return true; }
			virtual void set_inductance(double a_inductance) { inductance(a_inductance); }
		  public:
			double inductance() const { return m_inductance; }
			void inductance(double a_inductance){ m_inductance = a_inductance; }
		};

		class Gate_Inverted : virtual public Configurable {
			bool m_gate_inverted;
			// context menu integration
			virtual bool needs_gate_inverted(bool &a_inverted){ a_inverted = gate_inverted(); return true; }
			virtual void set_gate_inverted(bool a_inverted){ gate_inverted(a_inverted); }
		  public:
			void gate_inverted(bool a_invert) { m_gate_inverted = a_invert; }
			bool gate_inverted() const { return m_gate_inverted; }
		};
	}
}
