//--------------------------------------------//
// EncodeVorbis Build And Test Script         //
// License: Public Domain (www.unlicense.org) //
//--------------------------------------------//

const fs = require('fs'), execSync = require('child_process').execSync;

var path_clang        = '"../../../wasm/llvm18/clang"';
var path_wasmopt      = '"../../../wasm/wasm-opt118"';
var path_wasm2c       = '"../../../wasm/wabt/wasm2c26"';
var path_oggzvalidate = '"../../../oggz-validate"';
var sysdir            = './sys';
var c_optflags        = '-Os -DNDEBUG';
var w_optflags        = '-O4';
var wasmfile          = 'EncodeVorbis.wasm';
var cppfile           = 'EncodeVorbis.wasm.cpp';
var hfile             = 'EncodeVorbis.wasm.cpp.h';
var testpcm           = 'test.pcm';

var cc1base = ' -cc1 -x c -triple wasm32-unknown-emscripten -emit-obj ' + c_optflags;
var muslincludes = ' -isystem'+sysdir+'/lib/libc/musl/src/include -isystem'+sysdir+'/lib/libc/musl/src/internal';
var compincludes = ' -isystem'+sysdir+'/include/compat';
var libcincludes = ' -isystem'+sysdir+'/include -isystem'+sysdir+'/lib/libc/musl/include -isystem'+sysdir+'/lib/libc/musl/arch/emscripten -isystem'+sysdir+'/lib/libc/musl/arch/generic';

var qsort_cmd    = path_clang + cc1base + muslincludes + compincludes + libcincludes + ' -o qsort.o    '+sysdir+'/qsort.c';
var qsort_nr_cmd = path_clang + cc1base + muslincludes + compincludes + libcincludes + ' -o qsort_nr.o '+sysdir+'/qsort_nr.c';
var dlmalloc_cmd = path_clang + cc1base                + compincludes + libcincludes + ' -o dlmalloc.o '+sysdir+'/dlmalloc.c -DMALLOC_FAILURE_ACTION=';
var wasm_cmd     = path_clang + ' -target wasm32-unknown-emscripten ' + c_optflags + ' -DENCODE_VORBIS_SKIP_ALL_CLEANUP'
	+ ' -nostartfiles -nodefaultlibs -Xlinker --no-entry -Xlinker -allow-undefined -Xlinker --color-diagnostics=never -Xlinker -strip-all' + libcincludes
	+ ' -Xlinker -export=__wasm_call_ctors -Xlinker -export=EncodeVorbis qsort.o qsort_nr.o dlmalloc.o EncodeVorbis.c -o ' + wasmfile;
var opt_cmd      = path_wasmopt + ' ' + w_optflags + ' ' + wasmfile + ' -o ' + wasmfile;
var w2c_cmd      = path_wasm2c + ' ' + wasmfile + ' -o ' + cppfile;

if (1 && path_clang)
{
	process.env.TMP = "."; // make clang store its temporary files in the current directory
	console.log("Compiling qsort        ...",    qsort_cmd); console.log(execSync(   qsort_cmd).toString());
	console.log("Compiling qsort_nr     ...", qsort_nr_cmd); console.log(execSync(qsort_nr_cmd).toString());
	console.log("Compiling dlmalloc     ...", dlmalloc_cmd); console.log(execSync(dlmalloc_cmd).toString());
	console.log("Compiling EncodeVorbis ...",     wasm_cmd); console.log(execSync(    wasm_cmd).toString());
	fs.unlinkSync('qsort.o'); fs.unlinkSync('qsort_nr.o'); fs.unlinkSync('dlmalloc.o');
	if (1 && path_wasmopt)
	{
		console.log("Optimizing...", opt_cmd);
		console.log(execSync(opt_cmd).toString());
	}
}
console.log("WASM Size: ", fs.statSync(wasmfile).size);

if (1 && path_wasm2c)
{
	console.log("transpiling...", w2c_cmd);
	console.log(execSync(w2c_cmd).toString());
	fs.unlinkSync(hfile); // throws error if hfile is wrong

	// convert wasm2c output to use our own EncodeVorbis.wasm-rt.h header
	var cpp = fs.readFileSync(cppfile, 'utf8');
	if (cpp.indexOf('#include "' + hfile + '"')  <0)throw'bad c'; cpp = cpp.replace('#include "' + hfile + '"',  '#include "EncodeVorbis.wasm-rt.h"');
	if (cpp.indexOf('#define UNLIKELY')          <0)throw'bad c'; cpp = cpp.replace('#define UNLIKELY',          '#ifdef WASM_RT_USE_TRAP\n#define UNLIKELY');
	if (cpp.indexOf('\n#if WABT_BIG_ENDIAN')     <0)throw'bad c'; cpp = cpp.replace('\n#if WABT_BIG_ENDIAN',     '#endif\n\n#ifdef WASM_RT_USE_GENLOADSTORE\n#if WABT_BIG_ENDIAN');
	if (cpp.indexOf('\n#define I32_CLZ')         <0)throw'bad c'; cpp = cpp.replace('\n#define I32_CLZ',         '#endif\n\n#define I32_CLZ');
	if (cpp.indexOf('#define DIV_S')             <0)throw'bad c'; cpp = cpp.replace('#define DIV_S',             '#ifdef WASM_RT_USE_TRAP\n#define DIV_S');
	if (cpp.indexOf('\n#define I32_DIV_S')       <0)throw'bad c'; cpp = cpp.replace('\n#define I32_DIV_S',       '#endif\n\n#define I32_DIV_S');
	if (cpp.indexOf('#define DIVREM_U')          <0)throw'bad c'; cpp = cpp.replace('#define DIVREM_U',          '#ifdef WASM_RT_USE_TRAP\n#define DIVREM_U');
	if (cpp.indexOf('\n#define DIV_U')           <0)throw'bad c'; cpp = cpp.replace('\n#define DIV_U',           '#endif\n\n#define DIV_U');
	if (cpp.indexOf('#define FMIN')              <0)throw'bad c'; cpp = cpp.replace('#define FMIN',              '#ifdef WASM_RT_USE_TRAP\n#define FMIN');
	if (cpp.indexOf('\n#define I32_TRUNC_S_F32') <0)throw'bad c'; cpp = cpp.replace('\n#define I32_TRUNC_S_F32', '#endif\n\n#define I32_TRUNC_S_F32');
	if (cpp.indexOf('(wasm_rt_elem_t){')         <0)throw'bad c'; cpp = cpp.replace(/\(wasm_rt_elem_t\){/g, '{');
	fs.writeFileSync("../" + cppfile, cpp);
	fs.unlinkSync(cppfile);
}

if (1 && testpcm && fs.existsSync(testpcm))
{
	const wasmBuffer = fs.readFileSync(wasmfile), pcmSamples = new Int16Array(fs.readFileSync(testpcm).buffer), oggBase = 'test';
	var pcmOffset = 0, mem, mem_bytes, mem_view, heapEnd, oggFile;

	const env =
	{
		exit(arg) { throw 'exit called' },
		sbrk(incr)
		{
			incr = (incr + 15) & ~15; // align to 16 bytes boundry
			var oldHeapEnd = heapEnd, newHeapEnd = heapEnd + incr, newPages = (newHeapEnd + 65535) >> 16, addPages = (newPages - (mem_bytes.length >> 16));
			if (addPages > 0)
			{
				mem.grow(addPages);
				mem_bytes = new Uint8Array(mem.buffer);
				mem_view = new DataView(mem.buffer);
			}
			heapEnd = newHeapEnd;
			return oldHeapEnd;
		},
		memcpy(dest, src, count)
		{
			if (dest <= 0) throw 'invalid memcpy dest';
			mem_bytes.set(mem_bytes.subarray(src, src + count), dest);
			return dest;
		},
		memmove(dest, src, count)
		{
			if (dest <= 0) throw 'invalid memmove dest';
			if (dest <= src)
				for (var i = dest, j = src, end = dest + count; i != end;) mem_bytes[i++] = mem_bytes[j++];
			else
				for (var i = dest + count - 1, j = src + count - 1; i >= dest;) mem_bytes[i--] = mem_bytes[j--];
			return dest;
		},
		memset(dest, ch, count)
		{
			if (dest <= 0) throw 'invalid memset dest';
			mem_bytes.fill(ch, dest, dest + count);
			return dest;
		},
		log(v) { return Math.log(v); },
		cos(v) { return Math.cos(v); },
		sin(v) { return Math.sin(v); },
		exp(v) { return Math.exp(v); },
		atan(v) { return Math.atan(v); },
		pow(v, w) { return Math.pow(v, w); },
		abs(v) { return Math.abs(v); },
		labs(v) { return Math.abs(v); },
		ldexp(mantissa, exponent)
		{
			// JavaScript imeplementation of ldexp from https://blog.codefrau.net/2014/08/deconstructing-floats-frexp-and-ldexp.html
			var steps = Math.min(3, Math.ceil(Math.abs(exponent) / 1023)), result = mantissa;
			for (var i = 0; i < steps; i++)
				result *= Math.pow(2, Math.floor((exponent + i) / steps));
			return result;
		},
		EncodeVorbisOutput(ptr_data, len)
		{
			if (oggFile) fs.writeSync(oggFile, mem_bytes, ptr_data, len)
		},
		EncodeVorbisFeedSamples(ptr_buffer_arr, num)
		{
			var leftPtr  = mem_view.getUint32(ptr_buffer_arr+0, true);
			var rightPtr = mem_view.getUint32(ptr_buffer_arr+4, true);
			var remain = ((pcmSamples.length - pcmOffset) / 2);
			if (remain < num) num = remain;
			//console.log('EncodeVorbisFeedSamples', pcmOffset, '/', pcmSamples.length );
			for (var pcmOffsetEnd = pcmOffset + num * 2, i = 0; pcmOffset != pcmOffsetEnd; leftPtr += 4, rightPtr += 4)
			{
				mem_view.setFloat32(leftPtr, pcmSamples[pcmOffset++] / 32768.0, true);
				mem_view.setFloat32(rightPtr, pcmSamples[pcmOffset++] / 32768.0, true);
			}
			return num;
		},
	};

	WebAssembly.instantiate(wasmBuffer, { env: env }).then(wasmModule =>
	{
		mem = wasmModule.instance.exports.memory;
		heapEnd = mem.buffer.byteLength;
		mem_bytes = new Uint8Array(mem.buffer);
		mem_view = new DataView(mem.buffer);
		wasmModule.instance.exports.__wasm_call_ctors();
		for (var q = 0; q <= 10; q++)
		{
			console.log("\nEncoding at quality " + q + " ...\n-----------------------------------------------------------------------------------------------------")
			oggFile = fs.openSync(oggBase+q+'.ogg', 'w'); pcmOffset = 0;

			console.time('encode');
			try { wasmModule.instance.exports.EncodeVorbis(q); } catch (e) { console.log(e); }
			console.timeEnd('encode');

			fs.closeSync(oggFile); oggFile = null;
			console.log("-----------------------------------------------------------------------------------------------------\n");

			if (1 && path_oggzvalidate && fs.existsSync(path_oggzvalidate))
			{
				console.log("\nValidating file ...\n-----------------------------------------------------------------------------------------------------")
				try { console.log(execSync(path_oggzvalidate + ' ' + oggBase+q+'.ogg').toString()); } catch (e) { console.log(e.output.toString()); }
				console.log("-----------------------------------------------------------------------------------------------------\n");
			}
		}
	});
}
