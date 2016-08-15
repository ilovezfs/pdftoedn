TESTS = \
	test_arg_page_negative.sh \
	test_arg_page_out_of_range.sh \
	test_arg_missing_output_file.sh \
	test_arg_fontmap_does_not_exist.sh \
	test_arg_invalid_fontmap_file_json_syntax.sh \
	test_arg_invalid_fontmap_file_no_fontmaps.sh \
	test_arg_invalid_pdf.sh \
	test_diff_output.sh

AM_TESTS_ENVIRONMENT = \
	TESTS_DIR='$(abs_top_srcdir)/tests'; export TESTS_DIR; \
	PDFTOEDN='$(top_builddir)/src/pdftoedn$(EXEEXT)'; export PDFTOEDN;

ref-edn: $(top_builddir)/src/pdftoedn$(EXEEXT)
	sh ./generate_ref_edn.sh $(top_builddir)/src/pdftoedn$(EXEEXT)

clean-local:
	-rm -f *.old *.tmp docs/*.edn
