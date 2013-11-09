/*
 * pi.h
 * Copyright (C) 2013 Tobias Markus <tobias@markus-regensburg.de>
 * 
 */

#ifndef PI_H
#define PI_H

#include <atomic>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cstdarg>
#include <cmath>
#include <stdexcept>
#include "mpreal.h"
#include "util.h"
#include "tsio.h"

namespace picalc
{
	struct run_info
	{
		size_t precision;
		unsigned int threads;
		run_info() \
			: precision(0), threads(0)
		{ }
		run_info(unsigned int p, unsigned int t) \
			: precision(p), threads(t)
		{ }
	};	

	class chudnovsky
	{
	private:
		static inline const mpfr::mpreal for_k(const unsigned int k)
		{
			// -1 ^ k
			// (6k)!
			// /
			// (k!)^3 * (3k)!
			// *
			// (13591409 + 545140134k)
			// /
			// 640320^3k
			mpfr::mpreal a =	mpfr::pow(-1.0, k) *			\
			(mpfr::fac_ui(6.0 * k) * (13591409.0 + (545140134.0 * k)))	\
			/								\
			(mpfr::fac_ui(3.0 * k) * mpfr::pow(mpfr::fac_ui(k), 3.0) *	\
			mpfr::pow(640320.0, 3.0 * k));
		

			return a;
		}
		static inline const mpfr::mpreal pi_for(const mpfr::mpreal sum)
		{
			return mpfr::pow((mpfr::sqrt(10005.0) / 4270934400.0) * sum, (-1.0));
		}
		static inline const mpfr::mpreal exp_mod(const mpfr::mpreal b, mpfr::mpreal n, const mpfr::mpreal k)
		{
			mpfr::mpreal r = 1.0;
			mpfr::mpreal t = 0.0;
			unsigned int i;
			for (i = 0; t <= n; i++)
			{
				t = mpfr::pow(2.0, i);
			}
			i--;
			t = mpfr::pow(2.0, i);
			while (1)
			{
				if (n >= t)
				{
					r = mpfr::mod(b * r, k);
					n -= t;
				}
				t /= 2.0;
				if (t >= 1.0)
				{
					r = mpfr::mod(mpfr::pow(r, 2.0), k);
				}
				else
					break;
			}
			return r;
		}
		const mpfr::mpreal sum_for(const mpfr::mpreal n, const mpfr::mpreal j)
		{
			mpfr::mpreal sum;

			for (mpfr::mpreal k = 0; k <= n; k++)
			{
				//sum += mpfr::mod(mpfr::pow(16.0, n - k), (8.0 * k) + j) / ((8.0 * k) + j);
				sum += exp_mod(16.0, n - k, (8.0 * k) + j) / ((8.0 * k) + j);
			}
			for (mpfr::mpreal k = n + 1.0; k <= n + 32.0; k++)
			{
				sum += mpfr::pow(16.0, n - k) / (8.0 * k + j);
			}

			return sum;
		}
		const mpfr::mpreal bbp_for(const mpfr::mpreal n)
		{
			mpfr::mpreal a =	(4.0 * sum_for(n, 1)) \
					-	(2.0 * sum_for(n, 4)) \
					-	(1.0 * sum_for(n, 5)) \
					-	(1.0 * sum_for(n, 6));
			a = mpfr::mod(a, 1.0);
			return a;
		}
		std::mutex m;
		run_info info;
		unsigned int threadc;
		std::vector<std::thread> t;
		std::string ieee_float_to_hex(float f)
		{
			static_assert(std::numeric_limits<float>::is_iec559, "native float must be an IEEE float");

			union { float fval; std::uint32_t ival; };
			fval = f;

			std::ostringstream stm;
			stm << std::hex << std::uppercase << ival;

			return stm.str();
		}

		unsigned int hex_at(const mpfr::mpreal x, unsigned int k, unsigned int len = 8)
		{
			/*std::string s = r.toString();
		std::cout << s << std::endl;
			unsigned int idx = s.find('.');
		std::cout << idx << std::endl;
			s = s.substr(idx + 1);
			k -= idx + 1;
		std::cout << s << std::endl;
			unsigned int x = std::stoi(s.substr(k, 8));
		std::cout << x << std::endl;*/

			/*for (unsigned int j = 1; j < k; j++)
			{
				//s = s.insert(0, )
			}*/
			std::cout << "1" << std::endl;
			mpfr::mpreal r = x;
			r -= mpfr::trunc(r);
			mpfr::mpreal tmp = r;
			std::string result;
			std::cout << "2" << std::endl;
			for (; len > 0; len--)
			{
				r *= 16.0;
				result.append(to_string<unsigned int>((unsigned long)mpfr::trunc(r).toULong(), std::hex));
				r -= mpfr::trunc(r);
				if (r == 0.0)
					break;
			}
			std::cout << "3" << std::endl;
			//std::cout << result << std::endl;

			std::cout << result << std::endl;
			std::cout << result.substr(k, 8) << " " << k << std::endl;
			return std::stoi(result.substr(k, 8), nullptr, 16);
			
			/*float tmp = ldexp(std::stof(s.substr(0, 2)) * 0.01, 4);
			float last;
			std::string result;
			for (unsigned int j = 1; j < k; j++)
			{
				tmp = ldexp(std::stof(s.substr(j, 2)), 4);
				result.append(to_string<unsigned int>(tmp, std::hex));
				last = tmp - std::trunc(tmp);
			}*/

			//return 0;
		}
	protected:
	public:
		void calculate(const unsigned int runs)
		{
			mpfr::mpreal::set_default_prec(info.precision);
			mpfr::mpreal sum = 0;

			for (unsigned int ph = 0; ph < threadc; ph++)
			{
				t[ph] = std::thread( [&] (unsigned int phase)
					{
						for (unsigned int k = phase; k < runs; k += threadc)
						{
							mpfr::mpreal tmp = for_k(k);
							std::unique_lock<std::mutex> lock (m);
							//std::cout << tmp << std::endl;
							sum += tmp;
						}
					}, ph);
			}

			//std::cout.precision(16);
			/*for (unsigned int k = 0; k < 10; k++)
			{
				std::cout << bbp_for(k) << std::endl;
				printf("%014lx\n", (unsigned long int)(bbp_for(k).toLDouble() * 72057594037927936.0));
			}*/
			join_all(t);
			mpfr::mpreal pi = pi_for(sum);

			unsigned long correct_digits = 0;
			for (unsigned int k = 0; k < pi.toString().size(); k++)
			{
			//	std::cout << std::hex << hex_at(bbp_for(k), 0, 1) << std::endl;
			//	std::cout << std::hex << bbp_for(k).toLDouble() * 72057594037927936.0 << std::endl << hex_at(pi, k) * 72057594037927936.0 << std::endl;
			//	std::cout << std::hex << bbp_for(k).toString().substr(0, 1) << std::endl << std::to_string(hex_at(pi, k)).substr(0, 1) << std::endl;
				std::cout << "a" << std::endl;
				if (hex_at(bbp_for(k), 0, 1) == hex_at(pi, 0, 1))
					correct_digits++;
				else
					break;
				std::cout << "b" << std::endl;
				/*if (bbp_for(k).toString().substr(0, 1) == std::to_string(hex_at(pi, k)).substr(0, 1))
					correct_digits++;*/
			}
			std::cout.precision(14);
			std::cout << std::hex << (unsigned long int)(bbp_for(0).toLDouble() * 72057594037927936.0) << std::dec << std::endl;
			std::cout << "Correct digits: " << correct_digits << std::endl;
			std::cout.precision(1024);
			std::stringstream ss;
			ss << pi;
			std::cout << ss.str() << std::endl;
			std::cout << "Buffer Length: " << ss.str().size() << std::endl;
			std::cout << std::hex << hex_at(pi, 0) << std::endl;
			printf("%014lx\n", (unsigned long int)(bbp_for(0).toLDouble() * 72057594037927936.0));
		}
		chudnovsky(const run_info r) : info(r), threadc(r.threads), t(threadc)
		{
		}
		~chudnovsky()
		{
		}
	};

	class pi
	{
	private:
		//bool finished;
	public:
		/*friend std::ostream& operator<<(std::ostream& out, const pi& p) noexcept
		{
			if (p.finished == false)
				throw std::logic_error("Run picalc::calculate() before trying to output pi!");
			mp_exp_t exponent;
			//std::string outstr = mpf_get_str(NULL, &exponent, 10, 0, p.actual.get_mpf_t());
			//out << outstr << "e" << exponent;
			return out;
		}
		size_t digits() const noexcept
		{
			if (finished == false)
				throw std::logic_error("Run picalc::calculate() before trying to output pi!");
			//mp_exp_t exponent;
			//std::string outstr = mpf_get_str(NULL, &exponent, 10, 0, actual.get_mpf_t());
			std::stringbuffer str;
			return outstr.size();
		}*/
		pi(const run_info r, unsigned int runs) //: finished(false)
		{
			chudnovsky ch(r);
			ch.calculate(runs);
		}
		~pi()
		{
		}
		/*void calculate(const unsigned int runs)
		{
			cout << "Calculating..." << endl;
			e.calculate(runs);
			cout << "Constructing future..." << endl;
			std::future<mpf_class> f = std::async(std::launch::async,
				[&] ()
				{
					actual = calc->actual();
					return actual;
				});
			std::thread t(
				[&] ()
				{
					actual = e.actual();
					return actual;
				});
			cout << "Waiting..." << endl;
			double prog;
			do
			{
				prog = calc->get_progress();
				print_percent(prog);
			} while (prog < 1);
			print_percent(1);
			f.wait();*/
			//t.join();
			//cout << endl << "All threads are finished." << endl;
			//finished = true;
		
	};
};

#endif
