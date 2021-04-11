# home.tpl
<div class="container">
	<ul>
	{{ #results }}
		<li>{{ .category }}</li>
		<li>{{ .header }}</li>
	{{ /results }}
	</ul>
</div>
