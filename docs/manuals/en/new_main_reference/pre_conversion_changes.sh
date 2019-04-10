#!/bin/bash

SOURCEFILE=$1
DESTFILE=$2

while IFS= read -r line
do
  # \input{director-resource-director-definitions.tex}
  if  [[ "${line}" =~ \\input\{(.*)\} ]]; then
    INPUTFILE="../main/${BASH_REMATCH[1]}"

    if [ ! -f "${INPUTFILE}" ] ; then
      INPUTFILE="${INPUTFILE}.tex"
    fi


    if [ -f "${INPUTFILE}" ] ; then
      echo "%%%% INPUT OF ${INPUTFILE} start"
      cat "${INPUTFILE}"
      echo "%%%% INPUT OF ${INPUTFILE} end"
    else
      echo "%%%% INPUTFILE ${INPUTFILE} NOT FOUND"
    fi
  else
    echo "${line}"
  fi
done < "${SOURCEFILE}" > "${DESTFILE}"

mv "${DESTFILE}" "${DESTFILE}.orig"

cat ${DESTFILE}.orig\
    | perl -pe 's#\\begin\{bmessage\}#\\begin{verbatim}\\begin{bmessage}#g' \
    | perl -pe 's#\\end\{bmessage\}#\\end{bmessage}\\end{verbatim}#g' \
\
    | perl -pe 's#\\begin\{logging\}#\\begin{verbatim}\\begin{logging}#g' \
    | perl -pe 's#\\end\{logging\}#\\end{logging}\\end{verbatim}#g' \
\
    | perl -pe 's#\\begin\{config\}#\\begin{verbatim}\\begin{config}#g' \
    | perl -pe 's#\\end\{config\}#\\end{config}\\end{verbatim}#g' \
\
    | perl -pe 's#\\begin\{bconfig\}#\\begin{verbatim}\\begin{bconfig}#g' \
    | perl -pe 's#\\end\{bconfig\}#\\end{bconfig}\\end{verbatim}#g' \
\
    | perl -0 -pe 's|(\\begin\{commands\}.*?\\end\{commands\})|\\begin{verbatim}\1\\end{verbatim}|smg' \
\
    | perl -pe 's#\\begin\{bareosConfigResource\}#\\begin{verbatim}\\begin{bareosConfigResource}#g' \
    | perl -pe 's#\\end\{bareosConfigResource\}#\\end{bareosConfigResource}\\end{verbatim}#g' \
\
    | perl -pe 's#\\begin\{commandOut\}#\\begin{verbatim}\\begin{commandOut}#g' \
    | perl -pe 's#\\end\{commandOut\}#\\end{commandOut}\\end{verbatim}#g' \
\
    | perl -0 -pe 's|(\\begin\{bconsole\}.*?\\end\{bconsole\})|\\begin{verbatim}\1\\end{verbatim}|smg' \
\
    | perl -0 -pe 's|(\\begin\{tabular\}.*?\\end\{tabular\})|\# Tabular in LaTex format (original)\n\\begin{verbatim}\1\\end{verbatim}\n\n\# Tabular converted from LaTeX to RST (or empty, in case of problems):\n\1|smg' \
\
    | perl -0 -pe 's|\\subsubsubsection\{|\\paragraph\{|g' \
\
    | perl -0 -pe 's|\\releasenoteSection\{(.*?)\}|\\section*{\1}\n\n|smg' \
\
    | perl -0 -pe 's|\\releasenote\{(.*?)\}|\\subsection*{bareos-\1}\n\\index[general]{bareos-\1!Release Notes}\n\n|smg' \
\
    | perl -0 -pe 's|\\releasenoteUnstable\{(.*?)\}|\\subsection*{\\textit{bareos-\1 (unstable)}}\n\n|smg' \
\
    | perl -pe 's#\\variable\{\$#\\textbf\{\\\$#g' \
    | perl -pe 's#sec:#section-#g' \
    | perl -pe 's#\\hfill##g' \
    | perl -pe 's#\$CONFIGDIR#CONFIGDIR#g'\
    | perl -pe 's#\$COMPONENT#COMPONENT#g'\
    | perl -pe 's#\$RESOURCE#RESOURCE#g'\
    | perl -pe 's#\$NAME#NAME#g'\
    | perl -pe 's#\$PATH#PATH#g'\
    | perl -pe 's#\\hide\{\$\}##g' \
    | perl -pe 's#\\path\|#\\verb\|path:#g' \
    | perl -pe 's#\\path\!#\\verb\!path:#g' \
    | perl -pe 's#\\hypertarget#\textbf#g' \
    | perl -pe 's#\\bquote\{\\bconsoleOutput\{Building directory tree ...\}\}#\\bquote{Building directory tree ...}#g' \
    | perl -pe 's#\$\\ldots\$#...#g' \
    | perl -pe 's#\$<\$#<#g' \
    | perl -pe 's#\$<=\$#<=#g' \
    | perl -pe 's#\$>\$#>#g' \
    | perl -pe 's#\$>=\$#>=#g' \
    | perl -pe 's#\$\\le\$#<#g' \
    | perl -pe 's#\$\\leq\$#<=#g' \
    | perl -pe 's#\$\\ge\$#>#g' \
    | perl -pe 's#\$\\geq\$#>=#g' \
    | perl -pe 's#\$\\star\$#*#g' \
    | perl -pe 's#\{\\textbar\}#\|#g' \
    | perl -pe 's#\{\\textless\}#<#g' \
    | perl -pe 's#\{\\textgreater\}#>#g' \
    > ${DESTFILE}


# \newcommand{\releasenote}[2]{
#     \subsection*{bareos-#1}
#     \addcontentsline{toc}{subsection}{\protect\numberline{}#1}%
#     \label{bareos-#1}
#     \index[general]{bareos-#1!Release Notes}%
#     #2
# }


#    | perl -pe 's#\\begin\{bconsole\}#\\begin{verbatim}\\begin{bconsole}#g' \
#    | perl -pe 's#\\end\{bconsole\}#\\end{bconsole}\\end{verbatim}#g' \


# needs to get rid of $, as overwise pandoc gets confused.
# Therefore variables, \hide{$} get removed.
# \path|...| gets replaced, as pandoc gets confused, when a # appears in the path. This is common in the NDMP chapter.
# Preconverting it to verb helps on this.


#     | perl -pe 's#\\url\{(.*)\}#\\verb\|URL:\1\|#g' \


  diff  --color -ruN ${SOURCEFILE} ${DESTFILE}
